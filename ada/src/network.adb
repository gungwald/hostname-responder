with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;

with String_Functions; use String_Functions;

package body Network is

   EAGAIN : Integer;
   pragma Import (C, EAGAIN, "C_EAGAIN");
   EBADF  : Integer;
   pragma Import (C, EBADF,  "C_EBADF");

   function Get_Errno return Integer;
   pragma Import (C, Get_Errno, "Get_Errno");


   procedure Send_String(Sock    : in Socket_Type;
                         Message : in String;
                         Send_To : in Sock_Addr_Type)
   is
      Message_Stream : constant Stream_Element_Array := Convert_To_Stream_Elements(Message);
      Index_Of_Last_Elem_Sent : Stream_Element_Offset;
   begin
      Send_Socket(Sock, Message_Stream, Index_Of_Last_Elem_Sent, Send_To);
   end Send_String;
   

   procedure Receive_String(Sock                : in      Socket_Type;
                            Received_From       :     out Sock_Addr_Type;
                            Received_Message    :     out String;
                            Last_Index_Received :     out Natural)
   is
      Buffer : Stream_Element_Array(First_Index(Received_Message)..Last_Index(Received_Message));
      Last_Elem_Index_Received : Stream_Element_Offset;
   begin
      Receive_Socket(Sock, Buffer, Last_Elem_Index_Received, Received_From);
      Copy_String(Target => Received_Message, 
                  Source => Convert_To_String(Buffer, Last_Elem_Index_Received), 
                  Last   => Last_Index_Received);
   exception
      when e : others =>
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
      when e : others =>
         -- EBADF just means the socket hasn't been opened yet and that is normal
         -- if a shutdown is happening before a socket has been opened.
         if Get_Errno /= EBADF then
            Put_Line("Error closing socket: " & Exception_Message(e));
         end if;
   end Close_Socket_Continue;

end Network;
