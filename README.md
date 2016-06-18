# tftp

Basic client/server for TFTP (Trivial File Transfer Protocol).

Implementation started from scratch without using any library.

Respect theses RFC:
  * [RFC1350](https://tools.ietf.org/html/rfc1350): TFTP Protocol (Revision 2)
  * [RFC2347](https://tools.ietf.org/html/rfc2347): TFTP Option Extension. In particular:
    * [RFC2348](https://tools.ietf.org/html/rfc2348): TFTP Blocksize Option
    * [RFC2349](https://tools.ietf.org/html/rfc2349): TFTP Timeout Interval and Transfer Size Options
