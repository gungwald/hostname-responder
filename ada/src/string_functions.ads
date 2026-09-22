with Ada.Streams; use Ada.Streams;

package String_Functions is

   procedure Copy_String(Source : in String; Target : out String; Last : out Natural);
   function  Convert_To_String(Buffer : Stream_Element_Array; Last: Stream_Element_Offset) return String;
   function  Convert_To_Stream_Elements(s : String) return Stream_Element_Array;
   function  Convert_To_String(b : Boolean) return String;
   function  First_Index(s : String) return Stream_Element_Offset;
   function  Last_Index(s : String) return Stream_Element_Offset;

end String_Functions;
