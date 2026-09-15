with Ada.Exceptions; use Ada.Exceptions;
with Ada.Text_IO; use Ada.Text_IO;
with GNAT.Sockets; use GNAT.Sockets;
with Hwaet; use Hwaet;
with Network; use Network;

procedure Hwaetd is

   -- The Host_Name function does not make a call to DNS.
   My_Host_Name : constant String := GNAT.Sockets.Host_Name;
   Server_Sock : Socket_Type;
   Server_Addr : constant Sock_Addr_Type := (Family=>Family_Inet, Addr=>Any_Inet_Addr, Port=>Server_Port);
   Client_Sock : Socket_Type;
   Stopping : Boolean := False;
   Send_Offset : constant Stream_Element_Offset := 0;
     
   procedure Cleanup is
   begin
      Close_Socket_Continue(Server_Sock);
      Close_Socket_Continue(Client_Sock);
   end Cleanup;

begin

   Create_Socket(Server_Sock, Family_Inet, Socket_Datagram);
   Bind_Socket(Server_Sock, Server_Addr);
   Put_Line("Server bound to address: " & Image(Server_Addr));
   Create_Socket(Client_Sock, Family_Inet, Socket_Datagram);
   
   while not Stopping loop
      declare
         Client_Addr : Sock_Addr_Type;
         Received_Message : String(1..256);
         Response_Message : String(1..256);
         Response_Stream : Stream_Element_Array;
         Last : Natural;
      begin
         Receive_String(Server_Sock, Client_Addr, Received_Message, Last);
         Put_Line("Received message from: " & Image(Client_Addr.Addr) & ": " & Received_Message(1..Last));
         Client_Addr.Port := Client_Port;
         if Client_Addr.Family = Family_Inet then
            if Received_Message = Hwaet_Message then
               Copy_String(My_Host_Name, Response_Message, Last);
            elsif Received_Message = "" then
               Copy_String("An empty request is invalid", Response_Message, Last);
            else
               Copy_String("Invalid request: " & Received_Message, Response_Message, Last);
            end if;
         else
            Copy_String("Request must be IPv4", Response_Message, Last);
         end if;
         Response_Stream := Convert_To_Stream_Elements(Response_Message(1..Last));
         Send_Socket(Client_Sock, Response_Stream, Send_Offset, Client_Addr);
      end;      
   end loop;
   
exception
   when e : others =>
      Put_Line(Exception_Information(e));
      Cleanup;
end Hwaetd;

