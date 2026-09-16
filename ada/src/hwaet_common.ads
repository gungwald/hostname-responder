with Ada.Characters.Latin_1;
with Ada.Streams; use Ada.Streams;

with GNAT.Sockets; use GNAT.Sockets;

with String_Functions;

package Hwaet_Common is

   Server_Port : constant Port_Type := 4140;
   Client_Port : constant Port_Type := 4141;
   
   -- Hwæt form that is immune to source code character set issues.
   Hwaet_Message : constant String := "Hw" & Ada.Characters.Latin_1.LC_AE_Diphthong & "t";
   Hwaet_Stream : constant Stream_Element_Array := String_Functions.Convert_To_Stream_Elements(Hwaet_Message);

   Packet_Length : constant Natural := 256;

end Hwaet_Common;
