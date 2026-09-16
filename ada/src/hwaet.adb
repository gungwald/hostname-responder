-- Hwæt

with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;

with GNAT.Sockets; use GNAT.Sockets;

with Hwaet_Common; use Hwaet_Common;
with Network; use Network;
with Network_Interfaces; use Network_Interfaces;
with Terminal_Control; use Terminal_Control;


-- Hwæt is an Old English exclamation. Pronounced "who-wat" but as one
-- syllable. This program broadcasts a request to the subnet for all hosts
-- to respond with their hostname and IP address. It's like saying "Hello"
-- or "Who goes there?". It's similar to Avahi except that it will actually
-- work when you run it.

procedure Hwaet is

   -- ********************
   -- *                  *
   -- * Global Variables *
   -- *                  *
   -- ********************

   Broadcast_Sock : Socket_Type;
   Broadcast_Addr : constant Sock_Addr_Type := (Family=>Family_Inet,Addr=>Inet_Addr(Find_Broadcast_Address),Port=>SERVER_PORT);

   Receiver_Sock: Socket_Type;
   Receiver_Addr: constant Sock_Addr_Type := (Family=>Family_Inet,Addr=>Any_Inet_Addr,Port=>CLIENT_PORT);

   Index_Of_Last_Elem_Sent : Stream_Element_Offset;
   
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
   Set_Socket_Option(Broadcast_Sock, Socket_Level, (Broadcast,True));

   -- Because the Bcast_Sock does a broadcast, it can't receive a response.
   -- So this is done with two different sockets on two different ports.

   -- Setup receiver
   Create_Socket(Receiver_Sock, Family_Inet, Socket_Datagram);
   Bind_Socket(Receiver_Sock, Receiver_Addr);
   Set_Socket_Option(Receiver_Sock, Socket_Level, (Receive_Timeout,10.0));

   -- Do the work.
   Send_Socket(Broadcast_Sock, Hwaet_Stream, Index_Of_Last_Elem_Sent, Broadcast_Addr);
   Put_Line("Broadcast request to subnet. Waiting for responses...");
   loop
      declare
         Client_Addr: Sock_Addr_Type;
         Received_Message: String(1..Packet_Length);
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

