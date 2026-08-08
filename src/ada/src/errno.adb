
package body Errno is

   function Get_Formatted_System_Error(Message : String) return String is
      Err_Code : constant int := GNAT_Errno;
      C_Msg    : constant Strings.chars_ptr := c_strerror(Err_Code);
      Ada_Msg  : constant String := Message & " (" &  int'Image(Err_Code) & "): " & Strings.Value (C_Msg);
   begin
      return Ada_Msg;
   end Get_Error_Desc;

end Errno;
