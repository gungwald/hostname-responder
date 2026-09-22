package Trace is

procedure Start_Tracing;
procedure Stop_Tracing;
function  Tracing return Boolean;

procedure Trace_Entry(Proc_Or_Func_Name : String);
procedure Trace_Entry(Proc_Or_Func_Name : String; Arg_Name : String; Arg_Value : String);

procedure Trace_Message(Proc_Or_Func_Name : String; Message : String);

procedure Trace_Exit(Proc_Name : String);

procedure Trace_Return(Func_Name : String);
procedure Trace_Return(Func_Name : String; Value : String);

end Trace;
