-- Hwæt

with Ada.Exceptions; use Ada.Exceptions;
with Ada.Text_IO; use Ada.Text_IO;
with GNAT.Sockets; use GNAT.Sockets;
with Ada.Streams; use Ada.Streams;
with Ada.Characters.Latin_1; use Ada.Characters.Latin_1;

-- Hwaet is an Old English greeting. Pronounced "what" but starting with an
-- "h" sound? This program broadcasts a request to the subnet for all hosts
-- to respond with their hostname and IP address. It's like saying "Hello"
-- or "Who goes there?". It's similar to Avahi except that it will actually
-- work when you run it.

procedure Hwaet is

   procedure Copy_String(Source : in     String;
                         Target :    out String;
                         Last   :    out Natural)
   is
      Copy_Length: constant Natural := Natural'Min(Source'Length, Target'Length);
      Source_Stop: constant Integer := Source'First + (Copy_Length - 1);
      Target_Stop: constant Integer := Target'First + (Copy_Length - 1);
   begin
      if Copy_Length > 0 then
         Target(Target'First..Target_Stop) := Source(Source'First..Source_Stop);
      end if;
      -- Pad any remaining positions in target.
      if Target'Length > Source'Length then
         Target(Target_Stop + 1 .. Target'Last) := (others => ' ');
      end if;
      Last := Target_Stop;
   end Copy_String;

   -- Extremely stupid Ada nonsense.
   function Convert_To_String(Buffer: in Stream_Element_Array;
                              Last:   in Stream_Element_Offset) return String
   is
   begin
      if Last >= Buffer'First then
         declare
            Length: constant Natural := Natural(Last - Buffer'First + 1);
            -- Overlay a string.
            Converted_String: String(1..Length);
            for Converted_String'Address use Buffer(Buffer'First)'Address;
            pragma Import(Ada, Converted_String);
         begin
            return Converted_String;
         end;
      end if;
      return "";
   end Convert_To_String;

   procedure Receive_String(Sock            : in     Socket_Type;
                            Client_Addr     :    out Sock_Addr_Type;
                            Received_String :    out String;
                            Last            :    out Natural)
   is
      Buffer: Stream_Element_Array(1..256);
      Offset: Stream_Element_Offset;
   begin
      Receive_Socket(Sock, Buffer, Offset, Client_Addr);
      Copy_String(Target => Received_String, 
                  Source => Convert_To_String(Buffer, Offset), 
                  Last => Last);
   end Receive_String;

   -- Prototypes?
   function To_Stream_Element_Array(s: String) return Stream_Element_Array;
--   function Image(Elements: Stream_Element_Array) return String;    
 
   -- Converts Stream_Element_Array to String
 --  function Image(Elements: Stream_Element_Array) return String
 --  is
 --     Result: String(1 .. 13) 
 --                with Import, Convention => Ada, Address => Elements'Address;
 --  begin
 --      return Result;
 --  end Image;
 
   function To_Stream_Element_Array (s: String) return Stream_Element_Array
   is
       Result: constant Stream_Element_Array(1 .. s'Size / 8);
       for Result'Address use s'Address;
       pragma Import (Convention => Ada, Entity => Result);
   begin
       return Result;
   end To_Stream_Element_Array;

-- ********************
-- *                  *
-- * Global Variables *
-- *                  *
-- ********************

   Control_Sequence_Introducer : constant String := Ada.Characters.Latin_1.ESC & "[";
   ANSI_Term_Bold: constant String := Control_Sequence_Introducer & "1m";
   ANSI_Term_Reset: constant String := Control_Sequence_Introducer & "0m";
   Bcast_Sock: Socket_Type;
   Bcast_Addr: Sock_Addr_Type;
   Bcast_Addr_Text: constant String := "192.168.1.255";
   Bcast_Msg: constant String := "Hwæt";
   Bcast_Msg_Stream: constant Stream_Element_Array := To_Stream_Element_Array(Bcast_Msg);
   Offset : Stream_Element_Offset;

   Bcast_Stream: Stream_Access;
   Bcast_Port: constant Port_Type := 4140;

   Recvr_Sock: Socket_Type;
   Recvr_Port: constant Port_Type := 4141;
   Recvr_Addr: Sock_Addr_Type;

-- ****************
-- *              *
-- * Main Program *
-- *              *
-- ****************

begin
   -- Setup sender
   Create_Socket(Bcast_Sock,Family_Inet,Socket_Datagram);
   Bcast_Addr.Addr := Inet_Addr(Bcast_Addr_Text);
   Bcast_Addr.Port := Bcast_Port;
   Set_Socket_Option(Bcast_Sock,Socket_Level,(Broadcast,True));
   --Connect_Socket(Bcast_Sock,Bcast_Addr);
   --Bcast_Stream := Stream(Bcast_Sock);

   -- Because the Bcast_Sock does a broadcast, it can't receive a response.
   -- So this is done with two different sockets on two different ports.

   -- Setup receiver
   Create_Socket(Recvr_Sock,Family_Inet,Socket_Datagram);
   Recvr_Addr.Addr := Any_Inet_Addr;
   Recvr_Addr.Port := Recvr_Port;
   Bind_Socket(Recvr_Sock, Recvr_Addr);
   Set_Socket_Option(Recvr_Sock,Socket_Level,(Receive_Timeout,10.0));

   -- Do the work.
   Send_Socket(Bcast_Sock, Bcast_Msg_Stream, Offset, Bcast_Addr);

   --Broadcast_String(Bcast_Sock, Bcast_Addr, Bcast_Msg);
   --String'Output(Bcast_Stream,Bcast_Msg);
   Put_Line("Broadcast hostname request to subnet. Waiting for responses.");
   loop
      declare
         Client_Addr: Sock_Addr_Type;
         Recvd_Msg: String(1..256);
         Last: Natural;
      begin
         -- Loop will end when a socket read timeout occurs here.
         Receive_String(Recvr_Sock, Client_Addr, Recvd_msg, Last);
         Put_Line(ANSI_Bold & Image(Client_Addr.Addr) & ANSI_Reset & ": " & Recvd_Msg(1..Last));
      end;
  end loop;
exception when e: others =>
   Put_Line(Exception_Name(e) & ": " & Exception_Message(e));
   Close_Socket(Recvr_Sock);
   Close_Socket(Bcast_Sock);
end Hwaet;

