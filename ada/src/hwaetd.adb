with Ada.Command_Line; use Ada.Command_Line;
with Ada.Streams; use Ada.Streams;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Text_IO; use Ada.Text_IO;
with GNAT.Sockets; use GNAT.Sockets;
with Hwaet_Common; use Hwaet_Common;
with Network; use Network;
with String_Functions; use String_Functions;
with Trace; use Trace;

procedure Hwaetd is

   -- The Host_Name function does not make a call to DNS.
   My_Host_Name : constant String := GNAT.Sockets.Host_Name;
   Server_Sock : Socket_Type;
   Server_Addr : constant Sock_Addr_Type := (Family=>Family_Inet, Addr=>Any_Inet_Addr, Port=>Server_Port);
   Client_Sock : Socket_Type;
   Stopping : Boolean := False;
     
   procedure Cleanup is
   begin
      Close_Socket_Continue(Server_Sock);
      Close_Socket_Continue(Client_Sock);
   end Cleanup;

begin
   if Argument_Count > 0 and then Argument(1) = "-t" then
      Start_Tracing;
   end if;
   
   Create_Socket(Server_Sock, Family_Inet, Socket_Datagram);
   Bind_Socket(Server_Sock, Server_Addr);
   Put_Line("Server bound to address: " & Image(Server_Addr));
   Create_Socket(Client_Sock, Family_Inet, Socket_Datagram);
   
   while not Stopping loop
      declare
         Client_Addr : Sock_Addr_Type;
         Received_Message : String(1..Packet_Length);
         Response_Message : String(1..Packet_Length);
         Last_Index_Received : Natural;
         Last_Index_Copied : Natural;
         Last_Response_Index_Sent : Stream_Element_Offset;
      begin
         Receive_String(Server_Sock, Client_Addr, Received_Message, Last_Index_Received);
         Put_Line("Rcvd from=" & Image(Client_Addr.Addr) & " text=" & Received_Message(1..Last_Index_Received));
         Client_Addr.Port := Client_Port;
         if Client_Addr.Family = Family_Inet then
            if Received_Message(1..Last_Index_Received) = Hwaet_Message then
               Copy_String(My_Host_Name, Response_Message, Last_Index_Copied);
            elsif Last_Index_Received = 0 then
               Copy_String("Invalid empty request: ", Response_Message, Last_Index_Copied);
            else
               Copy_String("Invalid request: " & Received_Message, Response_Message, Last_Index_Copied);
            end if;
         else
            Copy_String("Invalid protocol: Only IPv4 is supported", Response_Message, Last_Index_Copied);
         end if;
         declare
            Response_Stream : Stream_Element_Array := Convert_To_Stream_Elements(Response_Message(1..Last_Index_Copied));
         begin
            Send_Socket(Client_Sock, Response_Stream, Last_Response_Index_Sent, Client_Addr);
            Put_Line("Sent to=" & Image(Client_Addr.Addr) & " text=" & Convert_To_String(Response_Stream, Last_Response_Index_Sent));
         end;
      end;      
   end loop;
   
exception
   when e : others =>
      Put_Line(Exception_Information(e));
      Cleanup;
end Hwaetd;

