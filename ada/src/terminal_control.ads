with Ada.Characters.Latin_1; use Ada.Characters.Latin_1;

package Terminal_Control is

   Control_Sequence_Introducer : constant String := Ada.Characters.Latin_1.ESC & "[";

   function ANSI_Terminal_Bold return String;
   function ANSI_Terminal_Reset return String;
   function Use_ANSI_Sequences return Boolean;

end Terminal_Control;

