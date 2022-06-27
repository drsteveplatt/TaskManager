//
// Non-blocking Read
//
// Read from Serial into a String.  Processes <backspace>.
// Works from "Serial Monitor" as well as from a PuTTY(etc)-style Telnet session.
#if !defined(__TM_READLINE_DEFINED__) 
#define __TM_READLINE_DEFINED__

/*! \file TaskManagerReadline.h */

/*!	\defgroup macros Task Manager Macros
	@{
*/

/*!	\brief 	Read a line from a stream into a String; non-blocking.
	Reads in a single line (terminated by \\r or \\n or \\r\\n)
	Processes backspaces.  Optionally echoes chars back to the input stream.
	
	Note that this routine uses the TM_YIELD macro.  As such, it needs to be
	supplied with a yield_id value to use in the TM_YIELD.  Also, any routine
	using this will need to be a TaskManager task and use one of the TM_BEGIN
	family of macros along with the accompanying TM_END.
	
	\param stream_in -- the input stream.  Also the output stream if echoing.
	\param string_out -- a String object. NOT cleared at entry.
	\param yield_id -- a unique integer to be used for the contained TM_YIELD
	\param echo -- true if chars are to be echoed back to the input stream.
*/
#define TM_READLINE(stream_in, string_out, yield_id, echo)			\
  { /* read in a whole line of chars into cmd	*/					\
    char ch;														\
    bool done;														\
    bool lastWasCr;													\
    cmd = "";														\
    done = false;													\
    lastWasCr = false;												\
    while(!done) {													\
      while(!stream_in.available()) { TM_YIELD(yield_id); }			\
      ch = stream_in.read();										\
      /* Backspace processing for PuTTY	*/							\
      if(ch==0x08 && echo) {										\
        if(cmd.length()>0) {										\
          Serial << '\b' << ' ' << '\b';							\
          cmd.remove(cmd.length()-1);								\
        }															\
      } else {														\
        /* CR processing for PuTTY */								\
        if(ch=='\r') { ch='\n'; lastWasCr = true; }					\
        else if(ch=='\n' && lastWasCr) { lastWasCr=false; continue; } /* crlf, skip the lf */		\
        if(ch=='\n') { Serial << ch; lastWasCr = false; done = true; }								\
        else {														\
          if(echo)Serial << ch;										\
          string_out += ch;											\
          lastWasCr = false;										\
        } /* end if(ch=='\n') else */								\
      } /* end if(ch==0x08) else */									\
    } /* end while */												\
  } /* end read in a whole line of chars */

/*! @} */ // end macros
#endif