with Ada.Characters.Latin_1;
with Ada.Streams; use Ada.Streams;

with String_Functions;

package Hwaet is

   Server_Port : constant Natural := 4140;
   Client_Port : constant Natural := 4141;
   
   -- Hwæt form that is immune to source code character set issues.
   Hwaet_Message : constant String := "Hw" & Ada.Characters.Latin_1.LC_AE_Diphthong & "t";
   Hwaet_Stream : constant Stream_Element_Array := String_Functions.Convert_To_Stream_Elements(Hwaet_Message);

end Hwaet;
