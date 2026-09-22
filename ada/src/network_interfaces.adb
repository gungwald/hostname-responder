with Ada.Command_Line; use Ada.Command_Line;
with Ada.Exceptions; use Ada.Exceptions;
with GNAT.OS_Lib; use GNAT.OS_Lib;

with Trace; use Trace;

package body Network_Interfaces is

   -- Returns false if Network_Interface is null.
   function Is_Primary_Interface(Network_Interface : ifaddrs_ptr) return Boolean is
      Name : constant String := Convert_To_String(Network_Interface.all.ifa_name);
      Flags : constant unsigned := Network_Interface.all.ifa_flags;
      Is_Broadcast : constant Boolean := (Flags and IFF_BROADCAST) > 0; -- Bitwise and
      Is_Loopback : constant Boolean := (Flags and IFF_LOOPBACK) > 0;   -- Bitwise and
      Family : unsigned_char;
      Result : Boolean;
   begin
      if Network_Interface.all.ifa_addr /= null then
         Family := Network_Interface.all.ifa_addr.all.sa_family;
         Trace_Message("Is_Primary_Interfae", "Family="&Family'Img & " Name="&Name & " Broadcast="&Is_Broadcast'Img & " Loopback="&Is_Loopback'Img);
         Result := Family = AF_INET and Is_Broadcast and not Is_Loopback;
      else
         Result := False;
      end if;
      Trace_Return("Is_Primary_Interface", Result'Img);
      return Result;
   end Is_Primary_Interface;


   -- Would fail if s is not terminated, depending on what Value does.
   function Convert_To_String(s : chars_ptr) return String is
   begin
      if s = Null_Ptr then
         return "";
      end if;
      declare
         Converted : constant String := Value(s);
      begin
         Trace_Return("Convert_To_String", Converted);
         return Converted;
      end;
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
      Trace_Return("Find_Primay_Interface", Convert_To_String(Primary_Interface.ifa_name)); -- Trace just the name
      return Primary_Interface;
   end Find_Primary_Interface;


   function Convert_To_String(Addr : in_addr) return String is
      -- Do not make this a constant, as suggested by the compiler because
      -- it will cause a storage error at runtime. Haha, Ada was wrong...
      IP_Addr_Text : aliased char_array(0 .. INET_ADDRSTRLEN - 1) := (others => nul);
      IP_Addr_Buf_Size : constant size_t := size_t(IP_Addr_Text'Size / 8);
      Addr_To_Convert : aliased in_addr := Addr; -- Need a copy to pass with Unchecked_Access
   begin
      Trace_Entry("Convert_To_String");
      if inet_ntop(int(AF_INET), Addr_To_Convert'Unchecked_Access, IP_Addr_Text, IP_Addr_Buf_Size) = Null_Ptr then
         Raise_Exception(Network_Interface_Error'Identity, "Failed to convert IP address to string: " & Errno_Message(Errno, ""));
      end if;
      declare
         Converted : constant String := To_Ada(IP_Addr_Text, Trim_Nul => True);
      begin
         Trace_Return("Convert_To_String", Converted);
         return Converted;
      end;
   end Convert_To_String;


   function Convert_To_Sockaddr_In(Generic_Sockaddr : sockaddr) return sockaddr_in is
      IPv4_Sockaddr_In : aliased sockaddr_in;
      for IPv4_Sockaddr_In'Address use Generic_Sockaddr'Address;
      pragma Import (Ada, IPv4_Sockaddr_In);
   begin
      if Generic_Sockaddr.sa_family /= AF_INET then
         Raise_Exception(Network_Interface_Error'Identity, "Non-IPv4 sockaddr cannot be converted to sockaddr_in");
      end if;
      Trace_Return("Convert_To_Sockaddr_In");
      return IPv4_Sockaddr_In;
   end Convert_To_Sockaddr_In;


   function Find_Broadcast_Address return String is
      Broadcast_Sockaddr : constant sockaddr_ptr := Find_Primary_Interface.ifa_broadaddr;
      Broadcast_Sockaddr_In : constant sockaddr_in := Convert_To_Sockaddr_In(Broadcast_Sockaddr.all);
      Broadcast_Addr : constant in_addr := Broadcast_Sockaddr_In.sin_addr;
      s : constant String := Convert_To_String(Broadcast_Addr);
   begin
      Trace_Return("Find_Broadcast_Address", s);
      return s;
   exception
      when e : others =>
         Raise_Exception(Network_Interface_Error'Identity, "Failed to find broadcast address: " & Exception_Information(e));
   end Find_Broadcast_Address;

begin
   if Argument_Count > 0 and then Argument(1) = "-t" then
      Start_Tracing;
   end if;
end Network_Interfaces;

