with Ada.Exceptions; use Ada.Exceptions;
with Ada.Text_IO; use Ada.Text_IO;
with GNAT.Sockets; use GNAT.Sockets;

procedure Hwaet is
   Socket: Socket_Type;
   Broadcast_Address: Sock_Addr_Type;
   Broadcast_Address_Text: constant String := "255.255.255.255";
   Message: constant String := "REPLY WITH ADDRESS";
   UDP_Packet_Stream: Stream_Access;
begin
   Create_Socket(Socket,Family_Inet,Socket_Datagram);
   Broadcast_Address.Addr := Inet_Addr(Broadcast_Address_Text);
   Broadcast_Address.Port := 4144;
   Set_Socket_Option(Socket,Socket_Level,(Broadcast,True));
   Set_Socket_Option(Socket,Socket_Level,(Receive_Timeout,20.0));
   Connect_Socket(Socket,Broadcast_Address);
   UDP_Packet_Stream := Stream(Socket);
   String'Output(UDP_Packet_Stream,Message);
   loop
      declare
         -- TODO: Current server can't receive and send on the same socket.
         Response: constant String := String'Input(UDP_Packet_Stream);
      begin
         Put_Line(Response);
      end;
   end loop;
exception when e: others =>
   Put_Line(Exception_Name(e) & ": " & Exception_Message(e));
   Close_Socket(Socket);
end Hwaet;

