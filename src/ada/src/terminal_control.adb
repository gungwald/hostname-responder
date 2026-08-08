with Ada.Environment_Variables;
with Interfaces.C_Streams;

package body Terminal_Control is

   package Env renames Ada.Environment_Variables;
   package C_IO renames Interfaces.C_Streams;

   ANSI_Enabled : Boolean;
   Bold         : constant String := Control_Sequence_Introducer & "1m";
   Reset        : constant String := Control_Sequence_Introducer & "0m";

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
      if ANSI_Enabled then
         return Bold;
      else
         return "";
      end if;
   end ANSI_Terminal_Bold;

   function ANSI_Terminal_Reset return String is
   begin
      if ANSI_Enabled then
         return Reset;
      else
         return "";
      end if;
   end ANSI_Terminal_Reset;

begin
   if Use_ANSI_Sequences then
      ANSI_Enabled := True;
   else
      ANSI_Enabled := False;
   end if;
end Terminal_Control;
