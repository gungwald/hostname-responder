with GNAT.Sockets; use GNAT.Sockets;
   
package Network is

   Socket_Read_Timeout : exception;

   procedure Receive_String(Sock               : in      Socket_Type;
                            Received_From_Addr :     out Sock_Addr_Type;
                            Received_String    :     out String;
                            Last               :     out Natural);
                            
   procedure Send_String(Sock           : in Socket_Type;
                         Message        : in String;
                         Target_Address : in Sock_Addr_Type);

   procedure Close_Socket_Continue(Sock : in out Socket_Type);

end Network;
