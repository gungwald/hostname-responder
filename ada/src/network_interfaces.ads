with Interfaces;
with Interfaces.C; use Interfaces.C;
with Interfaces.C.Strings; use Interfaces.C.Strings;
with System;

package Network_Interfaces is


   -- Define a distinct, reusable network exception
   Network_Interface_Error : exception;


   -- Defined as char for BSD but should work elsewhere also, given that the definitions
   -- of sin_family and sa_family are also unsigned_char.
   AF_INET : constant unsigned_char := 2;
   
   IFF_BROADCAST : constant unsigned := 2;
   IFF_LOOPBACK  : constant unsigned := 8;
   

   type sockaddr_filler is array (1 .. 14) of aliased unsigned_char;
   type sockaddr is record
      sa_len    : unsigned_char; -- Only exists in BSD but the other files should work elsewhere.
      sa_family : unsigned_char;
      sa_data   : sockaddr_filler;
   end record;
   pragma Convention (C, sockaddr);
   type sockaddr_ptr is access all sockaddr;

   type in_addr is record
      s_addr : unsigned;
   end record;
   pragma Convention (C, in_addr);
   type in_addr_ptr is access all in_addr;

   type sockaddr_in_filler is array (1 .. 8) of aliased unsigned_char;
   type sockaddr_in is record
      sin_len    : unsigned_char; -- Only exists in BSD but the other files should work elsewhere.
      sin_family : unsigned_char;
      sin_port   : unsigned_short;
      sin_addr   : in_addr;
      sin_zero   : sockaddr_in_filler;
   end record;
   pragma Convention (C, sockaddr_in);
   type sockaddr_in_ptr is access all sockaddr_in;

   type ifaddrs;
   type ifaddrs_ptr is access all ifaddrs;
   type ifaddrs is record
      ifa_next      : ifaddrs_ptr;
      ifa_name      : chars_ptr;
      ifa_flags     : unsigned;
      ifa_addr      : sockaddr_ptr;
      ifa_netmask   : sockaddr_ptr;
      ifa_broadaddr : sockaddr_ptr;
      ifa_data      : System.Address;
   end record;
   pragma Convention (C, ifaddrs);

   GETIFADDRS_FAILURE : constant int := -1;
   GETIFADDRS_SUCCESS : constant int := 0;
   INET_ADDRSTRLEN    : constant size_t := 16;

   -- C declaration: int getifaddrs(struct ifaddrs **ifap)
   -- The above argument, ifap, is a pointer to a pointer to a struct. In Ada 
   -- this becomes
   -- "access to access to a record". The "access to a record" part must exist
   -- as a variable and we pass the access to that, hence the type "access
   -- ifaddrs_ptr", where ifaddrs_ptr is actually "access to ifaddrs". Access
   -- ptr are just synonyms.
   function getifaddrs(Interface_List : access ifaddrs_ptr) return int;
   pragma Import (C, getifaddrs, "getifaddrs");

   -- void freeifaddrs(struct ifaddrs *ifap)
   procedure freeifaddrs(Interface_List : ifaddrs_ptr);
   pragma Import (C, freeifaddrs, "freeifaddrs");

   function inet_ntop
     (Address_Family : int;
      Source_Address : in_addr_ptr;
      Destination    : char_array;
      Destination_Size : size_t) return chars_ptr;
   pragma Import (C, inet_ntop, "inet_ntop");
   
   -- Network to Host Short value converter
   function ntohs(Network_Value : unsigned_short) return unsigned_short;
   pragma Import (C, ntohs, "ntohs");

   function Find_Primary_Interface return ifaddrs;
   
   function Find_Broadcast_Address return String;

   function Convert_To_String(s : chars_ptr) return String;

end Network_Interfaces;

