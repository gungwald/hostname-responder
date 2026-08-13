with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Exceptions; use Ada.Exceptions;
with GNAT.OS_Lib; use GNAT.OS_Lib;
with GNAT.Sockets.Thin_Common; use GNAT.Sockets.Thin_Common;
with GNAT.Sockets.Thin; use GNAT.Sockets.Thin;
with Interfaces.C; use Interfaces.C;
with System;

package body Network_Interfaces is
            
   Network_Interface_Error : exception;

   -- Returns false if Network_Interface is null.
   function Is_Primary_Interface(Network_Interface : ifaddrs_ptr) return Boolean is
      Name : constant String := Convert_To_String(Network_Interface.all.ifa_name);
   begin
      return Network_Interface.all.ifa_addr.sa_family = AF_INET 
         and then Name /= "lo0" 
         and then Name /= "lo";
   end Is_Primary_Interface;


   -- Would fail if s is not terminated, depending on what Value does.
   function Convert_To_String(s : chars_ptr) return String is
   begin
      if s = Null_Ptr then
         return "";
      end if;
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
      return Primary_Interface;
   end Find_Primary_Interface;


   function Convert_To_String(Addr : in_addr) return String is
      Addr_CString : aliased char_array(0 .. INET_ADDRSTRLEN - 1);
      CString_Size : constant size_t := size_t(Addr_CString'Size / System.Storage_Unit);
      Addr_Ptr : aliased in_addr_ptr := Addr'Access;
   begin
      if inet_ntop(int(AF_INET), Addr_Ptr, Addr_CString, CString_Size) = Null_Ptr then
         Raise_Exception(Network_Interface_Error'Identity, "Failed to convert IP address to string: " & Errno_Message(Errno, ""));
      end if;
      return To_Ada(Addr_CString, Trim_Nul => True);
   end Convert_To_String;


   function Convert_To_Sockaddr_In(Generic_Sockaddr : sockaddr) return sockaddr_in is
      IPv4_Sockaddr_In : aliased sockaddr_in;
      for IPv4_Sockaddr_In'Address use Generic_Sockaddr'Address;
      pragma Import (Ada, IPv4_Sockaddr_In, "");
   begin
      if Generic_Sockaddr.sa_family /= AF_INET then
         Raise_Exception(Network_Interface_Error'Identity, "Non-IPv4 sockaddr cannot be converted to sockaddr_in");
      end if;
      return IPv4_Sockaddr_In;
   end Convert_To_Sockaddr_In;


   function Find_Broadcast_Address return String is
      Broadcast_Sockaddr : sockaddr_ptr := Find_Primary_Interface.ifa_broadaddr;
   begin
      if Broadcast_Sockaddr = null then
         Raise_Exception(Network_Interface_Error'Identity, "Primary interface has no broadcast address");
      end if;
      return Convert_To_String(Convert_To_Sockaddr_In(Broadcast_Sockaddr.all).sin_addr);
   end Find_Broadcast_Address;


end Network_Interfaces;
