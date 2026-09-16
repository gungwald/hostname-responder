with GNAT.Sockets; use GNAT.Sockets;
   
package Network is

   Socket_Read_Timeout : exception;

   procedure Receive_String(Sock                : in      Socket_Type;
                            Received_From       :     out Sock_Addr_Type;
                            Received_Message    :     out String;
                            Last_Index_Received :     out Natural);
                            
   procedure Send_String(Sock    : in Socket_Type;
                         Message : in String;
                         Send_To : in Sock_Addr_Type);

   procedure Close_Socket_Continue(Sock : in out Socket_Type);

end Network;
