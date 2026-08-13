MODULE app;

IMPORT Sockets;
FROM SYSTEM IMPORT ADR;
FROM Terminal IMPORT Write, WriteLn; (* Standard ISO/M2C basic character output *)

CONST
  PORT = 8080;
  BUFFER_SIZE = 1024;

(* Native helper to write plain string arrays safely via standard Write *)
PROCEDURE WriteStr (str: ARRAY OF CHAR);
VAR i: CARDINAL;
BEGIN
  i := 0;
  WHILE (i <= HIGH(str)) AND (str[i] <> THEDRAW.CHAR(0)) DO
    Write(str[i]);
    INC(i);
  END;
END WriteStr;

(* Local utility to dump numerical ports out to the console interface *)
PROCEDURE WriteCard (val: CARDINAL);
VAR 
  buf: ARRAY [0..15] OF CHAR;
  i, j: CARDINAL;
  t: CHAR;
BEGIN
  i := 0;
  IF val = 0 THEN
    Write('0');
    RETURN;
  END;
  WHILE val > 0 DO
    buf[i] := CHR(ORD('0') + (val MOD 10));
    val := val DIV 10;
    INC(i);
  END;
  (* Reverse string characters *)
  j := 0;
  WHILE j < (i DIV 2) DO
    t := buf[j];
    buf[j] := buf[i - 1 - j];
    buf[i - 1 - j] := t;
    INC(j);
  END;
  buf[i] := CHR(0);
  WriteStr(buf);
END WriteCard;

VAR
  serverSock, clientSock: Sockets.SocketHandle;
  status: Sockets.NetError;
  optFlag: INTEGER;
  bytesRead, bytesSent: CARDINAL;
  buffer: ARRAY [0..BUFFER_SIZE-1] OF CHAR;
  errorBuf: ARRAY [0..127] OF CHAR;

BEGIN
  WriteStr("Initializing IPv6 TCP Echo Server..."); WriteLn;

  (* 1. Create a modern IPv6 TCP Stream Socket *)
  status := Sockets.CreateSocket(Sockets.IPv6, Sockets.TCP, serverSock);
  IF status <> Sockets.Success THEN
    WriteStr("CRITICAL: Failed to instantiate socket structure."); WriteLn;
    RETURN;
  END;

  (* 2. Set SO_REUSEADDR so the port can immediately rebind on restart *)
  optFlag := 1;
  status := Sockets.SetSocketOption(serverSock, Sockets.SOL_SOCKET, Sockets.SO_REUSEADDR, 
                                    ADR(optFlag), SIZE(optFlag));
  IF status <> Sockets.Success THEN
    WriteStr("WARNING: Failed to apply SO_REUSEADDR option flag."); WriteLn;
  END;

  (* 3. Bind to all interfaces over the specified port *)
  status := Sockets.BindSocket(serverSock, Sockets.IPv6, PORT);
  IF status <> Sockets.Success THEN
    WriteStr("CRITICAL: Bind failed on port "); WriteCard(PORT); WriteLn;
    Sockets.GetSystemErrorMessage(errorBuf);
    WriteStr("OS Reason: "); WriteStr(errorBuf); WriteLn;
    Sockets.CloseSocket(serverSock);
    RETURN;
  END;

  (* 4. Listen for incoming client validation steps *)
  status := Sockets.ListenForConnections(serverSock, 5);
  IF status <> Sockets.Success THEN
    WriteStr("CRITICAL: Backlog entry failure."); WriteLn;
    Sockets.CloseSocket(serverSock);
    RETURN;
  END;

  WriteStr("Server successfully running on port "); WriteCard(PORT); 
  WriteStr(". Waiting for a connection..."); WriteLn;

  (* 5. Block and accept exactly one connection for validation test *)
  status := Sockets.AcceptConnection(serverSock, clientSock);
  IF status <> Sockets.Success THEN
    WriteStr("CRITICAL: Error establishing client handle connection."); WriteLn;
    Sockets.CloseSocket(serverSock);
    RETURN;
  END;

  WriteStr("Client connected! Entering single-session echo channel loop..."); WriteLn;

  (* 6. Main Transaction Echo Loop *)
  LOOP
    status := Sockets.RecvData(clientSock, ADR(buffer), BUFFER_SIZE, bytesRead);
    
    IF status = Sockets.ClosedByPeer THEN
      WriteStr("Client detached from context safely."); WriteLn;
      EXIT;
    ELSIF status <> Sockets.Success THEN
      WriteStr("Runtime Receive Error encountered."); WriteLn;
      Sockets.GetSystemErrorMessage(errorBuf);
      WriteStr("OS Reason: "); WriteStr(errorBuf); WriteLn;
      EXIT;
    END;

    (* Echo the payload buffer exactly back to sender *)
    status := Sockets.SendData(clientSock, ADR(buffer), bytesRead, bytesSent);
    IF status <> Sockets.Success THEN
      WriteStr("Runtime Send Error encountered."); WriteLn;
      EXIT;
    END;
  END;

  (* 7. Clean up resource handles *)
  Sockets.CloseSocket(clientSock);
  Sockets.CloseSocket(serverSock);
  WriteStr("Server execution pipeline wrapped up smoothly."); WriteLn;

END app.
