IMPLEMENTATION MODULE Sockets;

IMPORT Socket;
FROM SYSTEM IMPORT ADDRESS, ADR, SIZE;

(*$C
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
*)

PROCEDURE CreateSocket (family: FamilyType; proto: ProtocolType; VAR sock: SocketHandle): NetError;
VAR
  cFamily, cType, cProto: INTEGER;
  fd: INTEGER;
BEGIN
  IF family = IPv4 THEN cFamily := Socket.AF_INET; ELSE cFamily := Socket.AF_INET6; END;
  
  IF proto = TCP THEN
    cType := Socket.SOCK_STREAM;
    cProto := Socket.IPPROTO_TCP;
  ELSE
    cType := Socket.SOCK_DGRAM;
    cProto := Socket.IPPROTO_UDP;
  END;

  fd := Socket.socket(cFamily, cType, cProto);
  IF fd < 0 THEN
    sock := -1;
    RETURN SocketCreationError;
  END;
  sock := fd;
  RETURN Success;
END CreateSocket;

PROCEDURE BindSocket (sock: SocketHandle; family: FamilyType; port: CARDINAL): NetError;
VAR
  storage: ARRAY [0..127] OF SHORTCARD; (* Acts as storage backing for struct sockaddr_storage *)
  storagePtr: ADDRESS;
  res: INTEGER;
  cLen: CARDINAL;
BEGIN
  storagePtr := ADR(storage);
  res := 0;
  
  IF family = IPv4 THEN
    cLen := 16; (* sizeof(struct sockaddr_in) *)
    (*$C
    struct sockaddr_in *sin = (struct sockaddr_in *)storagePtr;
    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
    sin->sin_addr.s_addr = INADDR_ANY;
    *)
  ELSE
    cLen := 28; (* sizeof(struct sockaddr_in6) *)
    (*$C
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)storagePtr;
    memset(sin6, 0, sizeof(struct sockaddr_in6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(port);
    sin6->sin6_addr = in6addr_any;
    *)
  END;

  (* Use a generic cast to pass the pointer down to the low-level wrapper *)
  (*$C res = bind(sock, (struct sockaddr *)storagePtr, cLen); *)
  
  IF res < 0 THEN RETURN BindError; END;
  RETURN Success;
END BindSocket;

PROCEDURE ListenForConnections (sock: SocketHandle; backlog: INTEGER): NetError;
BEGIN
  IF Socket.listen(sock, backlog) < 0 THEN RETURN ListenError; END;
  RETURN Success;
END ListenForConnections;

PROCEDURE AcceptConnection (serverSock: SocketHandle; VAR clientSock: SocketHandle): NetError;
VAR
  rawAddr: Socket.sockaddr;
  addrLen: CARDINAL;
  fd: INTEGER;
BEGIN
  addrLen := 128; (* Provide plenty of room to receive either an IPv4 or IPv6 structure *)
  fd := Socket.accept(serverSock, rawAddr, addrLen);
  IF fd < 0 THEN
    clientSock := -1;
    RETURN AcceptError;
  END;
  clientSock := fd;
  RETURN Success;
END AcceptConnection;

PROCEDURE ConnectToServer (sock: SocketHandle; family: FamilyType; ipAddress: ARRAY OF CHAR; port: CARDINAL): NetError;
VAR
  storage: ARRAY [0..127] OF SHORTCARD;
  storagePtr: ADDRESS;
  ipStrPtr: ADDRESS;
  res, cLen: INTEGER;
BEGIN
  storagePtr := ADR(storage);
  ipStrPtr := ADR(ipAddress);
  res := 0;

  IF family = IPv4 THEN
    cLen := 16;
    (*$C
    struct sockaddr_in *sin = (struct sockaddr_in *)storagePtr;
    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
    inet_pton(AF_INET, (char *)ipStrPtr, &sin->sin_addr);
    *)
  ELSE
    cLen := 28;
    (*$C
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)storagePtr;
    memset(sin6, 0, sizeof(struct sockaddr_in6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(port);
    inet_pton(AF_INET6, (char *)ipStrPtr, &sin6->sin6_addr);
    *)
  END;

  (*$C res = connect(sock, (struct sockaddr *)storagePtr, cLen); *)
  
  IF res < 0 THEN RETURN ConnectError; END;
  RETURN Success;
END ConnectToServer;

PROCEDURE SendData (sock: SocketHandle; buffer: ADDRESS; bytesToSend: CARDINAL; VAR bytesSent: CARDINAL): NetError;
VAR res: INTEGER;
BEGIN
  res := Socket.send(sock, buffer, bytesToSend, 0);
  IF res < 0 THEN bytesSent := 0; RETURN SendError; END;
  bytesSent := VAL(CARDINAL, res);
  RETURN Success;
END SendData;

PROCEDURE RecvData (sock: SocketHandle; buffer: ADDRESS; maxBytes: CARDINAL; VAR bytesReceived: CARDINAL): NetError;
VAR res: INTEGER;
BEGIN
  res := Socket.recv(sock, buffer, maxBytes, 0);
  IF res < 0 THEN bytesReceived := 0; RETURN RecvError; END;
  IF res = 0 THEN bytesReceived := 0; RETURN ClosedByPeer; END;
  bytesReceived := VAL(CARDINAL, res);
  RETURN Success;
END RecvData;

PROCEDURE CloseSocket (VAR sock: SocketHandle);
VAR discard: INTEGER;
BEGIN
  IF sock >= 0 THEN
    discard := Socket.close(sock);
    sock := -1;
  END;
END CloseSocket;

PROCEDURE GetSystemErrorMessage (VAR dest: ARRAY OF CHAR);
VAR destPtr: ADDRESS; maxLen: CARDINAL;
BEGIN
  destPtr := ADR(dest);
  maxLen := HIGH(dest) + 1;
  (*$C strncpy((char *)destPtr, strerror(errno), maxLen); *)
  (*$C ((char *)destPtr)[maxLen - 1] = '\0'; *)
END GetSystemErrorMessage;

PROCEDURE TextToIp (family: FamilyType; text: ARRAY OF CHAR; VAR ip: IpAddr): NetError;
VAR textPtr, ipDataPtr: ADDRESS; res, cFamily: INTEGER;
BEGIN
  textPtr := ADR(text);
  res := 0;
  
  IF family = IPv4 THEN
    cFamily := Socket.AF_INET;
    ip.family := IPv4;
    ipDataPtr := ADR(ip.v4Addr);
  ELSE
    cFamily := Socket.AF_INET6;
    ip.family := IPv6;
    ipDataPtr := ADR(ip.v6Addr);
  END;

  (*$C res = inet_pton(cFamily, (char *)textPtr, ipDataPtr); *)
  IF res <= 0 THEN RETURN InvalidAddressFormat; END;
  RETURN Success;
END TextToIp;

PROCEDURE IpToText (ip: IpAddr; VAR dest: ARRAY OF CHAR): NetError;
VAR destPtr, ipDataPtr: ADDRESS; maxLen, cFamily: CARDINAL; cRes: ADDRESS;
BEGIN
  destPtr := ADR(dest);
  maxLen := HIGH(dest) + 1;
  cRes := NIL;

  IF ip.family = IPv4 THEN
    cFamily := Socket.AF_INET;
    ipDataPtr := ADR(ip.v4Addr);
  ELSE
    cFamily := Socket.AF_INET6;
    ipDataPtr := ADR(ip.v6Addr);
  END;

  (*$C cRes = (void *)inet_ntop(cFamily, ipDataPtr, (char *)destPtr, maxLen); *)
  IF cRes = NIL THEN RETURN InvalidAddressFormat; END;
  RETURN Success;
END IpToText;

PROCEDURE SetSocketOption (sock: SocketHandle; level: INTEGER; optionName: INTEGER; optionValue: ADDRESS; optionLen: CARDINAL): NetError;
VAR res: INTEGER;
BEGIN
  res := 0;
  (*$C res = setsockopt(sock, level, optionName, optionValue, optionLen); *)
  IF res < 0 THEN RETURN SocketOptionError; END;
  RETURN Success;
END SetSocketOption;

PROCEDURE GetSocketOption (sock: SocketHandle; level: INTEGER; optionName: INTEGER; optionValue: ADDRESS; VAR optionLen: CARDINAL): NetError;
VAR res: INTEGER;
BEGIN
  res := 0;
  (*$C res = getsockopt(sock, level, optionName, optionValue, (socklen_t *)&optionLen); *)
  IF res < 0 THEN RETURN SocketOptionError; END;
  RETURN Success;
END GetSocketOption;

END Sockets.
