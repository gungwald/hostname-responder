with Interfaces.C.Strings;
with System;

package Errno is
   -- Import standard C error string utilities
   function c_strerror (errnum : Interfaces.C.int) return Interfaces.C.Strings.chars_ptr
      with Import => True, Convention => C, External_Name => "strerror";

   -- GNAT-specific way to fetch the thread-safe global C errno value
   function GNAT_Errno return Interfaces.C.int
      with Import => True, Convention => C, External_Name => "__get_errno";
      -- Note: Use "errno" or "__errno" depending on platform if "__get_errno" is missing

   function Get_Formatted_System_Error(Message : String) return String;
end Errno;
