// NOT FOR RELEASE
// EXPERIMENTAL/TEST CODE

// TaskManager Boilerplates
//

// Various boilerplate utilities

// Non-blocking Read
// Read from a stream into a String
// This is useful in TM_Macro code, so it will need TM_BEGIN() and TM_END().
//	stream_in is the stream to use (normally Serial)
//	string_out is the string variable to place the results in
//	yield_id is the TM_MACRO ID for TM_YIELD
//	echo is a boolean -- if true, echo the characters back to stream_in as read in
// TM_READLINE(stream_in, string_out, yield_id, echo)
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
