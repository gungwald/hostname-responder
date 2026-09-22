with Ada.Strings; use Ada.Strings;
with Ada.Strings.Fixed; use Ada.Strings.Fixed;
with Trace; use Trace; 


package body String_Functions is


   -- I don't know why this function doesn't already exist.
   procedure Copy_String(Source : in     String;
                         Target :    out String;
                         Last   :    out Natural)
   is
      Copy_Length: constant Natural := Natural'Min(Source'Length, Target'Length);
      Source_Stop: constant Integer := Source'First + (Copy_Length - 1);
      Target_Stop: constant Integer := Target'First + (Copy_Length - 1);
   begin
      Trace_Entry("Copy_String", "Source", Source);
      if Copy_Length > 0 then
         Trace_Message("Copy_String", "Copy_Length=" & Trim(Copy_Length'Img, Left));
         Target(Target'First..Target_Stop) := Source(Source'First..Source_Stop);
      end if;
      -- Pad any remaining positions in target. Not sure if cycles should be 
      -- wasted on this step.
      if Target'Length > Source'Length then
         Trace_Message("Copy_String", "Padding to length: " & Trim(Target'Last'Img, Left));
         Target(Target_Stop + 1 .. Target'Last) := (others => ' ');
      end if;
      Last := Target_Stop;
      Trace_Message("Copy_String", "Last=" & Trim(Last'Img, Left));
      Trace_Exit("Copy_String");
   end Copy_String;


   -- Extremely stupid Ada nonsense.
   function Convert_To_String(Buffer: in Stream_Element_Array;
                              Last:   in Stream_Element_Offset) return String is
   begin
      if Last >= Buffer'First then
         declare
            Length: constant Natural := Natural(Last - Buffer'First + 1);
            -- Overlay a string.
            Converted_String: String(1..Length);
            for Converted_String'Address use Buffer(Buffer'First)'Address;
            pragma Import(Ada, Converted_String);
         begin
            return Converted_String;
         end;
      else
         return "";
      end if;
   end Convert_To_String;


   
   function Convert_To_Stream_Elements(s : String) return Stream_Element_Array
   is
      Converted : constant Stream_Element_Array(First_Index(s) .. Last_Index(s));
      for Converted'Address use s'Address;
      pragma Import (Convention => Ada, Entity => Converted);
   begin
      return Converted;
   end Convert_To_Stream_Elements;
   

   function Convert_To_String(b : Boolean) return String is
   begin
      if b then
         return "True";
      else
         return "False";
      end if;
   end;
   

   function First_Index(s : String) return Stream_Element_Offset
   is
   begin
      return Stream_Element_Offset(s'First);
   end First_Index;
   
   
   function Last_Index(s : String) return Stream_Element_Offset is
   begin
      return Stream_Element_Offset(s'Last);
   end Last_Index;


end String_Functions;
