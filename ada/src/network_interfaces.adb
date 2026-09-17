with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;
with GNAT.OS_Lib; use GNAT.OS_Lib;


package body Network_Interfaces is

   package C_UShort_IO is new Ada.Text_IO.Modular_IO (Num => Interfaces.C.unsigned_short);
   package C_UChar_IO is new Ada.Text_IO.Modular_IO (Num => Interfaces.C.unsigned_char);
   package Bool_IO is new Ada.Text_IO.Enumeration_IO (Boolean);

   
   procedure Put_Network_Interface(Network_Interface : ifaddrs_ptr) is
      Name : constant String := Convert_To_String(Network_Interface.all.ifa_name);
      Flags : constant unsigned := Network_Interface.all.ifa_flags;
      Family : unsigned_char;
      Is_Broadcast : constant Boolean := (Flags and IFF_BROADCAST) > 0; -- Bitwise and
      Is_Loopback : constant Boolean := (Flags and IFF_LOOPBACK) > 0;   -- Bitwise and
   begin
      if Network_Interface.all.ifa_addr /= null then
         Family := Network_Interface.all.ifa_addr.all.sa_family;
         Put("Family="); C_UChar_IO.Put(Family);
         Put(" Name="); Put(Name);
         Put(" Broadcast="); Bool_IO.Put(Is_Broadcast);
         Put(" Loopback=");  Bool_IO.Put(Is_Loopback);
         New_Line;
      else
         Put_Line(Name & " has a null address");
      end if;
   end Put_Network_Interface;
   
   -- Returns false if Network_Interface is null.
   function Is_Primary_Interface(Network_Interface : ifaddrs_ptr) return Boolean is
      Flags : constant unsigned := Network_Interface.all.ifa_flags;
      Is_Broadcast : constant Boolean := (Flags and IFF_BROADCAST) > 0; -- Bitwise and
      Is_Loopback : constant Boolean := (Flags and IFF_LOOPBACK) > 0;   -- Bitwise and
      Family : unsigned_char;
   begin
      Put_Network_Interface(Network_Interface);
      if Network_Interface.all.ifa_addr /= null then
         Family := Network_Interface.all.ifa_addr.all.sa_family;
         return Family = AF_INET and Is_Broadcast and not Is_Loopback;
      else
         return False;
      end if;
   end Is_Primary_Interface;


   -- Would fail if s is not terminated, depending on what Value does.
   function Convert_To_String(s : chars_ptr) return String is
   begin
      if s = Null_Ptr then
         return "";
      end if;
      Put("Converted chars to String");
      return Value(s);
   end Convert_To_String;


   -- It will not return null, and will raise an exception if no primary 
   -- interface is found or if there is an error retrieving the interface list.
   function Find_Primary_Interface return ifaddrs is
      Primary_Interface : ifaddrs;
      Cursor : ifaddrs_ptr := null;
      Interface_List : aliased ifaddrs_ptr;
      Found : Boolean := False;
   begin
      if getifaddrs(Interface_List'Access) = GETIFADDRS_SUCCESS then
         Cursor := Interface_List;
         while not Found and Cursor /= null loop
            if Is_Primary_Interface(Cursor) then
               Found := True;
               Primary_Interface := Cursor.all;
            end if;
            Cursor := Cursor.all.ifa_next;
         end loop;
         freeifaddrs(Interface_List);
         if not Found then
            Raise_Exception(Network_Interface_Error'Identity, "Primary network interface not found");
         end if;
      else
         declare
            Error_Message : constant String := "Failed to get network interface list: " & Errno_Message(Errno, "");
         begin
            Raise_Exception(Network_Interface_Error'Identity, Error_Message);
         end;
      end if;
      Put("Found primary interface");
      return Primary_Interface;
   end Find_Primary_Interface;


   function Convert_To_String(Addr : in_addr) return String is
      IP_Addr_Text : aliased char_array(0 .. INET_ADDRSTRLEN - 1) := (others => nul);
      IP_Addr_Buf_Size : constant size_t := size_t(IP_Addr_Text'Size / 8);
      Addr_To_Convert : aliased in_addr := Addr; -- Need a copy to pass with Unchecked_Access
   begin
      Put_Line("Entering Convert_To_String");
      if inet_ntop(int(AF_INET), Addr_To_Convert'Unchecked_Access, IP_Addr_Text, IP_Addr_Buf_Size) = Null_Ptr then
         Raise_Exception(Network_Interface_Error'Identity, "Failed to convert IP address to string: " & Errno_Message(Errno, ""));
      end if;
      Put("Converted in_addr to String");
      return To_Ada(IP_Addr_Text, Trim_Nul => True);
   end Convert_To_String;


   function Convert_To_Sockaddr_In(Generic_Sockaddr : sockaddr) return sockaddr_in is
      IPv4_Sockaddr_In : aliased sockaddr_in;
      for IPv4_Sockaddr_In'Address use Generic_Sockaddr'Address;
      pragma Import (Ada, IPv4_Sockaddr_In);
   begin
      if Generic_Sockaddr.sa_family /= AF_INET then
         Raise_Exception(Network_Interface_Error'Identity, "Non-IPv4 sockaddr cannot be converted to sockaddr_in");
      end if;
      Put("Returning from Convert_To_Sockaddr_In");
      return IPv4_Sockaddr_In;
   end Convert_To_Sockaddr_In;


   function Find_Broadcast_Address return String is
      Broadcast_Sockaddr : constant sockaddr_ptr := Find_Primary_Interface.ifa_broadaddr;
      Broadcast_Sockaddr_In : constant sockaddr_in := Convert_To_Sockaddr_In(Broadcast_Sockaddr.all);
      Broadcast_Addr : constant in_addr := Broadcast_Sockaddr_In.sin_addr;
      s : constant String := Convert_To_String(Broadcast_Addr);
   begin
      Put("Broadcast address = " & s);
      return s;
   exception
      when e : others =>
         Raise_Exception(Network_Interface_Error'Identity, "Failed to find broadcast address: " & Exception_Information(e));
   end Find_Broadcast_Address;


end Network_Interfaces;
