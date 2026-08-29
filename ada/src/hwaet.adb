-- Hwæt

with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Characters.Latin_1; use Ada.Characters.Latin_1;

with GNAT.Sockets; use GNAT.Sockets;

with Terminal_Control; use Terminal_Control;
with Network_Interfaces; use Network_Interfaces;
with String_Functions; use String_Functions;


-- Hwæt is an Old English exclamation. Pronounced "who-wat" but as one
-- syllable. This program broadcasts a request to the subnet for all hosts
-- to respond with their hostname and IP address. It's like saying "Hello"
-- or "Who goes there?". It's similar to Avahi except that it will actually
-- work when you run it.

procedure Hwaet is

   EAGAIN : Integer;
   pragma Import (C, EAGAIN, "C_EAGAIN");

   function Get_Errno return Integer;
   pragma Import (C, Get_Errno, "Get_Errno");

   Socket_Read_Timeout : exception;


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


   -- ********************
   -- *                  *
   -- * Global Variables *
   -- *                  *
   -- ********************

   Broadcast_Sock: Socket_Type;
   Broadcast_Addr: Sock_Addr_Type;
   Broadcast_Addr_Text: constant String := Find_Broadcast_Address;
   Broadcast_Msg: constant String := "Hw" & LC_AE_Diphthong & "t"; -- Hwæt form that is immune to character set issues. 
   Broadcast_Msg_Stream: constant Stream_Element_Array := Convert_To_Stream_Elements(Broadcast_Msg);
   Offset : Stream_Element_Offset;
   Broadcast_Port: constant Port_Type := 4140;

   Receiver_Sock: Socket_Type;
   Receiver_Port: constant Port_Type := 4141;
   Receiver_Addr: Sock_Addr_Type;

   procedure Cleanup is
   begin
      Close_Socket_Continue(Receiver_Sock);
      Close_Socket_Continue(Broadcast_Sock);
   end Cleanup;

-- ****************
-- *              *
-- * Main Program *
-- *              *
-- ****************

begin
   -- Setup sender
   Create_Socket(Broadcast_Sock, Family_Inet, Socket_Datagram);
   Broadcast_Addr.Addr := Inet_Addr(Broadcast_Addr_Text);
   Broadcast_Addr.Port := Broadcast_Port;
   Set_Socket_Option(Broadcast_Sock, Socket_Level, (Broadcast,True));

   -- Because the Bcast_Sock does a broadcast, it can't receive a response.
   -- So this is done with two different sockets on two different ports.

   -- Setup receiver
   Create_Socket(Receiver_Sock, Family_Inet, Socket_Datagram);
   Receiver_Addr.Addr := Any_Inet_Addr;
   Receiver_Addr.Port := Receiver_Port;
   Bind_Socket(Receiver_Sock, Receiver_Addr);
   Set_Socket_Option(Receiver_Sock, Socket_Level, (Receive_Timeout,10.0));

   -- Do the work.
   Send_Socket(Broadcast_Sock, Broadcast_Msg_Stream, Offset, Broadcast_Addr);
   Put_Line("Broadcast request to subnet. Waiting for responses...");
   loop
      declare
         Client_Addr: Sock_Addr_Type;
         Received_Message: String(1..256);
         Last: Natural;
      begin
         -- Loop will end when a socket read timeout occurs here.
         Receive_String(Receiver_Sock, Client_Addr, Received_Message, Last);
         Put_Line(ANSI_Terminal_Bold & Image(Client_Addr.Addr) & ANSI_Terminal_Reset & ": " & Received_Message(1..Last));
      end;
   end loop;
exception
   when e : Socket_Read_Timeout =>
      Put_Line(Exception_Message(e));
      Cleanup;
   when e : others =>
      Put_Line(Exception_Information(e));
      Cleanup;
end Hwaet;

