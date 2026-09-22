with Ada.Environment_Variables;
with Interfaces.C_Streams;

package body Terminal_Control is

   package Env renames Ada.Environment_Variables;
   package C_IO renames Interfaces.C_Streams;

   Formatting_Enabled : Boolean;
   Seq_To_Begin_Bold  : constant String := Control_Sequence_Introducer & "1m";
   Seq_To_Reset_All   : constant String := Control_Sequence_Introducer & "0m";

   function Is_Formatting_Enabled return Boolean is
   begin
      return Formatting_Enabled;
   end Is_Formatting_Enabled;
   
   function Use_ANSI_Sequences return Boolean is
   begin
      if Env.Exists ("CLICOLOR_FORCE") then
         if Env.Value ("CLICOLOR_FORCE") /= "0" then
            return True;
         end if;
      end if;
      if Env.Exists ("NO_COLOR") then
         return False;
      end if;
      if Env.Exists ("CLICOLOR") and then Env.Value ("CLICOLOR") = "0" then
         return False;
      end if;
      if C_IO.Isatty (C_IO.Fileno (C_IO.Stdout)) /= 0 then
         return True;
      else
         return False;
      end if;
   exception
      when others =>
         return False;
   end Use_ANSI_Sequences;

   function ANSI_Terminal_Bold return String is
   begin
      if Formatting_Enabled then
         return Seq_To_Begin_Bold;
      else
         return "";
      end if;
   end ANSI_Terminal_Bold;

   function ANSI_Terminal_Reset return String is
   begin
      if Formatting_Enabled then
         return Seq_To_Reset_All;
      else
         return "";
      end if;
   end ANSI_Terminal_Reset;
   
   function Bold(s:String) return String is
   begin
      return ANSI_Terminal_Bold & s & ANSI_Terminal_Reset;
   end Bold;

begin
   if Use_ANSI_Sequences then
      Formatting_Enabled := True;
   else
      Formatting_Enabled := False;
   end if;
end Terminal_Control;
