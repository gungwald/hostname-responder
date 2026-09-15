with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;

with String_Functions; use String_Functions;

package body Network is

   EAGAIN : Integer;
   pragma Import (C, EAGAIN, "C_EAGAIN");

   function Get_Errno return Integer;
   pragma Import (C, Get_Errno, "Get_Errno");


   procedure Send_String(Sock : in Socket_Type;
                         Message : in String;
                         Target_Address : in Sock_Addr_Type)
   is
      Message_Stream : constant Stream_Element_Array := Convert_To_Stream_Elements(Message);
      Zero_Offset : constant Stream_Element_Offset := 0;
   begin
      Send_Socket(Sock, Message_Stream, Zero_Offset, Target_Address);
   end Send_String;
   

   procedure Receive_String(Sock            : in      Socket_Type;
                            Client_Addr     :     out Sock_Addr_Type;
                            Received_String :     out String;
                            Last            :     out Natural)
   is
      Buffer: Stream_Element_Array(1..256);
      Offset: Stream_Element_Offset;
   begin
      Receive_Socket(Sock, Buffer, Offset, Client_Addr);
      Copy_String(Target => Received_String, 
                  Source => Convert_To_String(Buffer, Offset), 
                  Last => Last);
   exception
      when e: others =>
         if Get_Errno = EAGAIN then
            Raise_Exception(Socket_Read_Timeout'Identity, "Timed out waiting for responses");
         else
            Raise_Exception(Exception_Identity(e), "Receive_String failed: " & Exception_Message(e));
         end if;
   end Receive_String;


   procedure Close_Socket_Continue(Sock : in out Socket_Type) is
   begin
      Close_Socket(Sock);
   exception
      when e: others =>
         Put_Line("Error closing socket: " & Exception_Message(e));
   end Close_Socket_Continue;

end Network;
