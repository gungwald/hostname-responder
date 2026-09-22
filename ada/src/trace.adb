with Ada.Strings; use Ada.Strings;
with Ada.Strings.Fixed; use Ada.Strings.Fixed;
with Ada.Text_IO; use Ada.Text_IO;

with Terminal_Control; use Terminal_Control;

package body Trace is

   ENTR : constant String := "Entr";
   EXET : constant String := "Exit";
   MESG : constant String := "Mesg";
   RETN : constant String := "Retn";

   type Tracing_Status_Type is (On, Off);

   Tracing_Status : Tracing_Status_Type := Off;
   Prefix : constant String := Bold("TRACE");


   function Tracing return Boolean is
   begin
      return Tracing_Status = On;
   end Tracing;


   procedure Start_Tracing is
   begin
      Tracing_Status := On;
   end Start_Tracing;


   procedure Stop_Tracing is
   begin
      Tracing_Status := Off;
   end Stop_Tracing;


   procedure Trace_Entry(Proc_Or_Func_Name : String) is
   begin
      if Tracing_Status = On then
         Put(Prefix);
         Put(" ");
         Put(ENTR);
         Put(" ");
         Put(Bold(Proc_Or_Func_Name));
         New_Line;
      end if;
   end Trace_Entry;


   procedure Trace_Entry(Proc_Or_Func_Name : String; Arg_Name : String; Arg_Value : String) is
   begin
      if Tracing_Status = On then
         Put(Prefix);
         Put(" ");
         Put(ENTR);
         Put(" ");
         Put(Bold(Proc_Or_Func_Name));
         Put(" ");
         Put(Arg_Name);
         Put("=");
         Put(Bold(Trim(Arg_Value,Both)));
         New_Line;
      end if;
   end Trace_Entry;


   procedure Trace_Message(Proc_Or_Func_Name : String; Message : String) is
   begin
      if Tracing_Status = On then
         Put_Line(Prefix & " " & MESG & " " & Bold(Proc_Or_Func_Name) & " " & Message);
      end if;
   end Trace_Message;


   procedure Trace_Exit(Proc_Name : String) is
   begin
      if Tracing_Status = On then
         Put_Line(Prefix & " " & EXET & " " & Bold(Proc_Name));
      end if;
   end Trace_Exit;


   procedure Trace_Return(Func_Name : String) is
   begin
      if Tracing_Status = On then
         Put_Line(Prefix & " " & RETN & " " & Bold(Func_Name));
      end if;
   end Trace_Return;


   procedure Trace_Return(Func_Name : String; Value : String) is
   begin
      if Tracing_Status = On then
         Put_Line(Prefix & " " & RETN & " " & Bold(Func_Name) & " value=" & Bold(Value));
      end if;
   end Trace_Return;

end Trace;
