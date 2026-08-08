with Ada.Characters.Conversions;
with GNAT.Sockets; use GNAT.Sockets;
with GNAT.Sockets.Thin; use GNAT.Sockets.Thin;
with GNAT.Sockets.Thin_Common; use GNAT.Sockets.Thin_Common;
with Ada.Exceptions; use Ada.Exceptions;
with Errno; use Errno;
with Ada.Strings.Fixed; use Ada.Strings.Fixed;
with Interfaces.C; use Interfaces.C;
with Interfaces.C.Strings; use Interfaces.C.Strings;

package body Network_Interfaces is

   GETIFADDRS_FAILURE_RESULT : constant int := -1;
   GETIFADDRS_SUCCESS_RESULT : constant int := 0;

   function Is_Primary_Interface(Interface : ifaddrs_ptr) return Boolean is
   begin
      if Interface = null then
         return False;
      end if;
      if Interface /= null and then Interface.all.ifa_addr.sa_family = AF_INET
      and then Value(Interface.all.ifa_name) /= "lo0" 
      and then Value(Interface.all.ifa_name) /= "lo" then
         return True;
      else
         return False;
      end if;
   end Is_Primary_Interface;

   function Convert_To_Ada_In_Addr(sa : sockaddr) return In_Addr is
      Addr : In_Addr;
   begin
      Addr.S_B1 := sa.sa_data(3);
      Addr.S_B2 := sa.sa_data(4);
      Addr.S_B3 := sa.sa_data(5);
      Addr.S_B4 := sa.sa_data(6);
      return Addr;
   end Convert_To_In_Addr;

   function Find_Primary_Interface return ifaddrs is
      Primary_Interface : ifaddrs;
      Cursor : ifaddrs_ptr;
      Found : Boolean := False;
      Interface_List : ifaddrs_ptr;
   begin
      if getifaddrs(Interface_List'Access) = GETIFADDRS_SUCCESS_RESULT then
         Cursor := Interface_List;
         while not Found and Cursor /= null loop
            declare
               Interface_Name : constant String := Value(Cursor.all.ifa_name);
               Addr_Bytes : In_Addr;
            begin
               if Is_Primary_Interface(Cursor) then
                  Found := True;
                  Primary_Interface := Cursor.all;
               end if;
            end;
            Cursor := Cursor.all.ifa_next;
         end loop;
         freeifaddrs(Interface_List);
         if not Found then
            Raise_Exception(Network_Interface_Error'Identity, "Primary network interface not found");
         end if;
      else
         declare
            Error_Message : constant String := Get_Formatted_System_Error("Failed to get network interfaces");
         begin
            Raise_Exception(Network_Interface_Error'Identity, Error_Message);
         end;
      end if;
      return Primary_Interface;
   end Find_Primary_Interface;

   function Find_Broadcast_Address return String is
      Broadcast_C_Sockaddr : sockaddr := Find_Primary_Interface.ifa_broadaddr.all;
      Broadcast_Ada_In_Addr : In_Addr := Convert_To_Ada_In_Addr(Broadcast_C_Sockaddr);
      Broadcast_Ada_Inet_Addr : Inet_Addr_Type;
   begin
      To_Inet_Addr(Broadcast_Ada_In_Addr, Broadcast_Ada_Inet_Addr);
      return Image(Broadcast_Ada_Inet_Addr);
   end Find_Broadcast_Address;


end Network_Interfaces;
