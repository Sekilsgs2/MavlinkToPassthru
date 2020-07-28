//=================================================================================================  
//================================================================================================= 
//
//                                    S . P O R T   C L A S S
//
//================================================================================================= 
//=================================================================================================

//    forward declaration of utility functions

uint32_t  Abs(int32_t);
float     RadToDeg (float );
uint16_t  prep_number(int32_t, uint8_t, uint8_t);
uint32_t  bit32Extract(uint32_t,uint8_t, uint8_t); 
void      bit32Pack(uint32_t, uint8_t, uint8_t);
uint32_t  createMask(uint8_t, uint8_t);
int16_t   Add360(int16_t, int16_t);
float     wrap_360(int16_t);
int8_t    PWM_To_63(uint16_t);
void      nbdelay(uint32_t);
void      DisplayPrintln(String);
void      PrintByte(byte, bool);
void      PrintByteNon(byte);
void      PrintByteOut(byte);
void      PrintByteIn(byte);
void      PrintMavLiteUplink();
void      PrintFrPeriod(bool);
void      PrintFrPeriod(bool);
uint8_t   PX4FlightModeNum(uint8_t, uint8_t);

class     SPort 
{ 
    // Data members 
  public: 
    
  private:
  
    bool      printcrc = false;
    byte      nb, lb;                      // NextByt, LastByt of ByteStuff pair     
    int16_t   CRC;                         // CRC of frsky frame
    uint8_t   time_slot_max = 16;              
    uint32_t  time_slot = 1;
    float     a, az, c, dis, dLat, dLon;
    uint8_t   sv_count = 0;

    volatile uint8_t *uartC3;
    enum SPortMode { rx , tx };
    SPortMode mode, modeNow;
  
    static const uint16_t  sp_max = 32;           // low level s.port read buffer
    byte            sp_buff[sp_max];
    uint16_t        sp_idx = 0;



    //                      S  C  H  E  D  U  L  E  R

    // Give the S.Port table more space when status_text messages sent three times
    #if defined Send_status_Text_3_Times
      static const uint16_t sb_rows = 300;  // possible unsent sensor ids at any moment 
    #else 
     static const uint16_t sb_rows = 130;  
    #endif 
    uint16_t        sb_row = 0;
  
    typedef union {
    struct {
       uint32_t       passthru;       // 4B  passthru payload
       uint16_t       packing;        // 2B   
         };
       mt_payload_t   mavlite;        // 6B  mavlite payload
      }
      sb_payload_t;                          

    // Scheduler identity for both Mavlink and MavLite
    typedef struct  {
      uint16_t      msg_id;          // 2B - note Mavlink needs 2 byte msg_id
      uint8_t       sub_id;          // 1B
      uint32_t      millis;          // 4B - mS since boot
      uint8_t       mt_idx;          // 1B - index if it is a MavLite frame class
      uint8_t       inuse;           // 1B
      sb_payload_t  payload;         // 6B
    } sb_t;

    sb_t          sb[sb_rows];                    // Scheduler buffer table


    uint32_t        sb_buf_full_gear = 0;         // downlink
    uint32_t        mt_buf_full_gear = 0;        // downlink
    uint32_t        mt20_buf_full_gear = 0;      // uplink

    uint32_t        sr_pt_payload;                // 4B
    uint16_t        sb_unsent;                    // how many sb rows in use  
    uint16_t        mt_unsent;                    // how many mt rows in use  


    // Member function declarations
  public:    
    void          initialise();
    void          SendAndReceive(); 
    void          PushMessage(uint16_t msg_id, uint8_t sub_id);  
    void          ReportSPortOnlineStatus(); 
    uint16_t      MatchWaitingParamRequests(char * paramid);           
  private:   
    void          setMode(SPortMode mode);
    void          AddInCrc(uint8_t b);   
    byte          SafeRead();
    void          SafeSend(byte b, bool isPayload);
    void          BlindInject(); 
    void          InjectFrame();  
    uint16_t      PopNextFrame();  
    msg_class_t   msg_class_now(uint8_t msg_id);
    void          FrameAndSend(uint16_t idx);
    bool          DecodeAndUplinkMavLite();   
    bool          UnchunkMavLiteUplink();
    void          SendPassthruPacket(uint8_t sensor_id, uint16_t msg_id);
    void          SendMavlitePacket(uint8_t sensor_id, uint16_t msg_id);
    void          SendCrc();
    uint16_t      FirstEmptyMt20Row();
    void          PushToEmptyRow(uint16_t msg_id, uint8_t sub_id); 
    void          Push_Param_Val_016(uint16_t msg_id);   
    void          Push_Lat_800(uint16_t msg_id);  
    void          Push_Lon_800(uint16_t msg_id);
    void          Push_Text_Chunks_5000(uint16_t msg_id);
    void          Push_AP_status_5001(uint16_t msg_id);  
    void          Push_GPS_status_5002(uint16_t msg_id);
    void          Push_Bat1_5003(uint16_t msg_id);
    void          Push_Home_5004(uint16_t msg_id);
    void          Push_VelYaw_5005(uint16_t msg_id);
    void          Push_Atti_5006(uint16_t msg_id);   
    void          Push_Parameters_5007(uint16_t msg_id, uint8_t sub_id);
    void          Push_Bat2_5008(uint16_t msg_id);  
    void          Push_WayPoint_5009(uint16_t msg_id);
    void          Push_Servo_Raw_50F1(uint16_t msg_id);    
    void          Push_VFR_Hud_50F2(uint16_t msg_id);    
    void          Push_Wind_Estimate_50F3(uint16_t msg_id);
    void          Push_Rssi_F101(uint16_t msg_id);                                                            
    void          PrintPayload(msg_class_t msg_class); 
    void          PrintMavLiteUplink();
   
      
}; // end of class

    // External member functions
    //===================================================================

    void SPort::initialise()  {

      for (int i=0 ; i < sb_rows ; i++) {  // initialise S.Port table
        sb[i].msg_id = 0;
        sb[i].sub_id = 0;
        sb[i].millis = 0;
        sb[i].inuse = 0;
      }


      for (int i = 0 ; i < mt20_rows ; i++) {  // initialise MavLite uplink table
        mt20[i].inuse = 0;
      } 
      
    #if (defined ESP32) || (defined ESP8266) // ESP only
      int8_t frRx;
      int8_t frTx;
      bool   frInvert;

      frRx = Fr_rxPin;
      frTx = Fr_txPin;

      #if defined ESP_Onewire 
        bool oneWire = true;
      #else
         bool oneWire = false;
      #endif
       
      if ((oneWire) || (set.trmode == ground)) {
        frInvert = true;
        Debug.print("S.Port on ESP is inverted "); 
      } else {
        frInvert = false;
        Debug.print("S.PORT NOT INVERTED! Hw inverter to 1-wire required "); 
      }

      #if ( (defined ESP8266) || ( (defined ESP32) && (defined ESP32_SoftwareSerial)) )
  
          if (oneWire) {
            frRx = frTx;     //  Share tx pin. Enable oneWire (half duplex)
            Debug.printf("S.Port on ESP is 1-wire half-duplex on pin %d \n", frTx); 
          } else {
          if (set.trmode == ground) {
            Debug.printf("S.Port on ESP is 1-wire simplex on tx pin = %d\n", frTx);
          } else { 
            Debug.printf("S.Port on ESP is 2-wire on pins rx = %d and tx = %d\n", frRx, frTx);
            if ((set.trmode == air) || (set.trmode == relay)) {
              Debug.println("Use a 2-wire to 1-wire converter for Air and Relay Modes");
            }  
           }  
          }
          nbdelay(100);
          frSerial.begin(frBaud, SWSERIAL_8N1, frRx, frTx, frInvert);     // SoftwareSerial
          nbdelay(100);
          Debug.println("Using SoftwareSerial for S.Port");
          if (oneWire) {
            frSerial.enableIntTx(true);
          }  
      #else  // HardwareSerial
          frSerial.begin(frBaud, SERIAL_8N1, frRx, frTx, frInvert); 
          if (set.trmode == ground)  {                        
            Debug.printf("on tx pin = %d\n", frTx);
          } else  
          if ((set.trmode == air) || (set.trmode == relay)) {
            Debug.printf("on pins rx = %d and tx = %d\n", frRx, frTx);  
            Debug.println("Use a 2-wire to 1-wire converter for Air and Relay Modes");
          }  
   
      #endif
    #endif

    #if (defined TEENSY3X) 
      frSerial.begin(frBaud); // Teensy 3.x    tx pin hard wired
     #if (SPort_Serial == 1)
      // Manipulate UART registers for S.Port working
       uartC3   = &UART0_C3;  // UART0 is Serial1
       UART0_C3 = 0x10;       // Invert Serial1 Tx levels
       UART0_C1 = 0xA0;       // Switch Serial1 into single wire mode
       UART0_S2 = 0x10;       // Invert Serial1 Rx levels;
   
     //   UART0_C3 |= 0x20;    // Switch S.Port into send mode
     //   UART0_C3 ^= 0x20;    // Switch S.Port into receive mode
     #else
       uartC3   = &UART2_C3;  // UART2 is Serial3
       UART2_C3 = 0x10;       // Invert Serial1 Tx levels
       UART2_C1 = 0xA0;       // Switch Serial1 into single wire mode
       UART2_S2 = 0x10;       // Invert Serial1 Rx levels;
     #endif
 
      Debug.printf("S.Port on Teensy3.x inverted 1-wire half-duplex on pin %d \n", Fr_txPin); 
 
    #endif   
    } // end of member function
    
    //===================================================================

    void SPort::AddInCrc(uint8_t b) {
       CRC += b;          // add in new byte
       CRC += CRC >> 8;   // add in high byte overflow if any
       CRC &= 0xff;  // mask all but low byte, constrain to 8 bits
       if (printcrc) Debug.printf("AddIn %3d %2X\tCRC_now=%3d %2X\n", b, b, CRC, CRC);
    }
    //=======================================================================  
      
    void SPort::setMode(SPortMode mode) {   
    
    #if (defined TEENSY3X) 
      if(mode == tx && modeNow !=tx) {
        *uartC3 |= 0x20;                 // Switch S.Port into send mode
        modeNow=mode;
        #if defined Debug_SPort_Switching
          Debug.print("tx");
        #endif
      }
      else if(mode == rx && modeNow != rx) {   
        *uartC3 ^= 0x20;                 // Switch S.Port into receive mode
        modeNow=mode;
        #if defined Debug_SPort_Switching
          Debug.print("rx");
        #endif
      }
    #endif

    #if (defined ESP8266) || (defined ESP32) 
        if(mode == tx && modeNow !=tx) { 
          modeNow=mode;
          pb_rx = false;
          #if (defined ESP_Onewire) && (defined ESP32_SoftwareSerial)        
          frSerial.enableTx(true);  // Switch S.Port into send mode
          #endif
          #if defined Debug_SPort_Switching
            Debug.print("tx");
          #endif
        }   else 
        if(mode == rx && modeNow != rx) {   
          modeNow=mode; 
          pb_rx = true; 
          #if (defined ESP_Onewire) && (defined ESP32_SoftwareSerial)                  
          frSerial.enableTx(false);  // disable interrupts on tx pin     
          #endif
          #if defined Debug_SPort_Switching
            Debug.print("rx");
          #endif
        } 
    #endif
    }  // end of member function    

    //===================================================================

    byte SPort::SafeRead() {
      byte b;  
      byte prevb=0; 
      // scope resolution operator (::)
      SPort::setMode(rx);
      if (stuffbyte) {
        b = stuffbyte; 
        stuffbyte = 0;
    
         } else {

        b = frSerial.read();  

        //  Byte in frame has value 0x7E is changed into 2 bytes: 0x7D 0x5E
        //  Byte in frame has value 0x7D is changed into 2 bytes: 0x7D 0x5D
    
    
        if (b == 0x7D) {
          prevb = b;
          b = frSerial.read();
          if (b == 0x5E) {
            b = 0x7E;   // // replace 0x7D 5E pair with 0x7E
          } else {
            stuffbyte = b;  // else forward both
            b = prevb;
          }
        }

        if (b == 0x7D) {
          prevb = b;
          b = frSerial.read();
          if (b == 0x5D) {
            b = 0x7D;   // replace 0x7D 5D pair with 0x7D
          } else {
            stuffbyte = b;  // else forward both
            b = prevb;
          }
        }    

      }   
      #if (defined Debug_SPort_In) || (defined Debug_SPort)  
        PrintByteIn(b);
      #endif 
  
      delay(0); // yield to rtos for wifi & bt to get a sniff 
      return b;
    } // end of member function
    //===================================================================

    void SPort::SafeSend(byte b, bool isPayload) {
    #if (not defined inhibit_SPort)
      SPort::setMode(tx);

      //  B Y T E   S T U F F   
      //  Byte in frame has value 0x7E is changed into 2 bytes: 0x7D 0x5E
      //  Byte in frame has value 0x7D is changed into 2 bytes: 0x7D 0x5D
      if (isPayload) {
        if (b == 0x7E) {
          frSerial.write(0x7D);
          #if (defined Debug_SPort_Out) || (defined Debug_SPort) 
            PrintByteOut(b);
          #endif 
          frSerial.write(0x5E);
        } else if (b == 0x7D) {
          frSerial.write(0x7D);
          #if (defined Debug_SPort_Out) || (defined Debug_SPort) 
            PrintByteOut(b);
          #endif         
          frSerial.write(0x5D);    
        } else {
      
        frSerial.write(b); 
        }
      } else {
        frSerial.write(b);    
      }

     if (isPayload) {  // Add CRC
       SPort::AddInCrc(b);
     }

     #if (defined Debug_SPort_Out) || (defined Debug_SPort) 
        PrintByteOut(b);
      #endif 
      delay(0); // yield to rtos for wifi & bt to get a sniff
    
    #endif      
    }
    //===================================================================

    void SPort::BlindInject() {  // Downlink

      SPort::InjectFrame();     // Blind inject a passthru or MavLite frame    
 
    } 
    //====================================   D O W N L I N K  ========================================= 
    void SPort::InjectFrame() {  
           
      uint16_t nxt = PopNextFrame();

      if (nxt != 0xffff) {   // not empty
        SPort::FrameAndSend(nxt);  
      }
     }
    
    //===================================================================
    void SPort::SendAndReceive() {  

      if (set.trmode == ground) {
        if(mavGood  && ((millis() - blind_inject_millis) > 18)) {  
          SPort::BlindInject();                    // Blind inject frame into Taranis et al 
          blind_inject_millis=millis();
         }
      } else {

      // Read SPort Stream
      SPort::setMode(rx);

      uint8_t Byt = 0;
      uint8_t prevByt=0;
  
      while (frSerial.available())   {  // Receive sensor IDs from X receiver
        Byt =  SPort::SafeRead();
        if (sp_idx > sp_max -1) {
       //   Debug.println("S.Port from X Receiver read buff overflow ignored");
          sp_idx--;
        }

        if ((prevByt == 0x7E) && (Byt == 0x1B)) {   // Real-Time DIY downlink slot found, slot ours in, and send right now  
             SPort::InjectFrame();                  // Interleave a packet right now!    
             return;  
          }

        if (Byt == 0x7E) {
          if ((sp_buff[1] == 0x0D) && (sp_buff[2] == 0x30)) { // MavLite uplink match found

            DecodeAndUplinkMavLite();        

          }      
          sp_idx = 0;
        }   
        sp_buff[sp_idx] = Byt;
        sp_idx++;
        prevByt=Byt;
      } // end of while
     }  // end of air and relay modes 
    }   // back to main loop
    //===================================================================  
 
    uint16_t SPort::PopNextFrame() {

      uint32_t sb_now = millis();
      int16_t sb_age;
      int16_t sb_tier_age;
      int16_t sb_oldest_tier1 = 0; 
      int16_t sb_oldest_tier2 = 0; 
      int16_t sb_oldest       = 0;     
      uint16_t idx_tier1      = 0;                 // row with oldest sensor data
      uint16_t idx_tier2      = 0; 
      uint16_t idx            = 0; 

      // 2 tier scheduling. Tier 1 gets priority, tier2 (0x5000) only sent when tier 1 empty 
  
      // find the row with oldest sensor data = idx 
      sb_unsent = 0;  // how many rows in-use

      uint16_t i = 0;
      while (i < sb_rows) {  
    
        if (sb[i].inuse) {
          sb_unsent++;   
      
          sb_age = (sb_now - sb[i].millis); 
          sb_tier_age = sb_age - sb[i].sub_id;  

          if (sb[i].msg_id == 0x5000) {
            if (sb_tier_age >= sb_oldest_tier2) {
              sb_oldest_tier2 = sb_tier_age;
              idx_tier2 = i;
            }
          } else {
          if (sb_tier_age >= sb_oldest_tier1) {
            sb_oldest_tier1 = sb_tier_age;
            idx_tier1 = i;
            }   
          }
        } 
      i++;    
      } 
    
      if (sb_oldest_tier1 == 0) {            // if there are no tier 1 sensor entries
        if (sb_oldest_tier2 > 0) {           // but there are tier 2 entries
          idx = idx_tier2;                   // send tier 2 instead
          sb_oldest = sb_oldest_tier2;
        }
      } else {
        idx = idx_tier1;                    // if there are tier1 entries send them
       sb_oldest = sb_oldest_tier1;
      }
  
      //Debug.println(sb_unsent);           // limited detriment :)  

      if (sb_oldest == 0)  return 0xffff;  // flag the scheduler table as empty

      if ((SPort::msg_class_now(sb[idx].msg_id)) == passthru) {
          fr_payload = sb[idx].payload.passthru; 
      } else
      if ((SPort::msg_class_now(sb[idx].msg_id)) == mavlite) {
          mt_Payload = sb[idx].payload.mavlite; 
      }

      #if (defined Frs_Debug_Scheduler) || (defined MavLite_Debug_Scheduler)
        uint16_t msgid_filter;
        #if (defined MavLite_Debug_Scheduler)
          msgid_filter = 0x100;
        #else
          msgid_filter = 0xffff; // just high value
        #endif

        if (sb[idx].msg_id < msgid_filter) {
          Debug.print(sb_unsent); 
          Debug.printf("\tPop  row= %3d", idx );
          Debug.print("  msg_id=0x");  Debug.print(sb[idx].msg_id, HEX);
          if (sb[idx].msg_id < 0x1000) Debug.print(" ");
          Debug.printf("  sub_id= %2d", sb[idx].sub_id); 
      
          pb_rx=false;
          SPort::PrintPayload(SPort::msg_class_now(sb[idx].msg_id));
          Debug.printf("  age=%3d mS \n" , sb_oldest_tier1 );        
        }
      #endif

      return idx;  // return the index of the oldest frame
     }
    //===================================================================   

    msg_class_t  SPort::msg_class_now(uint8_t msg_id) {
      if ((msg_id >= 20) && (msg_id <=77)) {
        return mavlite; 
      } else {
        return passthru;
      }
     }
    //===================================================================    
    void SPort::FrameAndSend(uint16_t idx) {
      #if defined Frs_Debug_Period
        PrintFrPeriod(0);   
      #endif    

      if (sb[idx].msg_id == 0xF101) {

        #if (defined Frs_Debug_Rssi)
          PrintFrPeriod(0);    
          Debug.println(" 0xF101 sent");
        #endif
    
        if (set.trmode != relay) {   
          SPort::SafeSend(0x7E, false);    // not payload, don't stuff or crc
          SPort::SafeSend(0x1B, false);  
        }
      }

      #if defined Frs_Debug_Scheduler
        Debug.printf("Injecting frame idx=%d ", idx);
        SPort::PrintPayload(SPort::msg_class_now(sb[idx].msg_id)); 
        Debug.println();
      #elif defined Debug_MavLite_Scheduler
      if (sb[idx].msg_id < 0x100) {  
        Debug.printf("Injecting frame idx=%d ", idx);
        SPort::PrintPayload(SPort::msg_class_now(sb[idx].msg_id)); 
        Debug.println();
      }  
      #endif

      msg_class = SPort::msg_class_now(sb[idx].msg_id);
      if (msg_class == passthru) {
        SPort::SendPassthruPacket(0x1B, sb[idx].msg_id);   // fr_payload given value in PopNextFrame() above
        fr_payload = 0;                                    // clear the payload field 
      } else
  
      if (msg_class == mavlite) {
        SPort::SendMavlitePacket(0x32, sb[idx].msg_id);    // mt_Payload given value in PopNextFrame() above
        mt_Payload = {};                                   // clear payload struct
      }
  
      sb[idx].inuse = 0;                                   // 0=free for use, 1=occupied - for passthru and mavlite
   
     }

    //===========================   U P L I N K  ========================
 
    bool SPort::DecodeAndUplinkMavLite() {
    #if defined Support_MavLite  
      mt_seq = sp_buff[3];
      if (mt_seq == 0) {
        mt_paylth = sp_buff[4];
        mt_msg_id = sp_buff[5];

        mt_idx = 0;
        }
        switch (mt_msg_id) {                // Decode uplink mavlite messages according to msg-id
          
          case 20:    //  #20 or 0x14   PARAM_REQUEST_READ
            if (SPort::UnchunkMavLiteUplink()) {
              #if defined Debug_MavLite
                Debug.printf("MavLite #20 Param_Request_Read :%s:\n", mt_param_id);  
              #endif  
              strncpy(ap22_param_id, mt_param_id, 16);
              
              ::Param_Request_Read(-1, ap22_param_id); // Request Param Read using param_id, not index  

              mt20_row = SPort::FirstEmptyMt20Row();
           //   Debug.printf("mt20_row=%d\n", mt20_row);
              if (mt20_row == 0xffff) return false;  // mt20 table overflowed, ignore this message :(

              strncpy(mt20[mt20_row].param_id, ap22_param_id, 16);
              mt20[mt20_row].millis = millis(); 
              mt20[mt20_row].inuse = 1;

              #if (defined Debug_MavLite_SPort)
                PrintMavLiteUplink();
              #endif
                 
              return true;           
            }
            return false;
          case 23:    // #23 or 0x17  PARAM_SET
             if (SPort::UnchunkMavLiteUplink()) {
              #if defined Debug_MavLite
                Debug.printf("MavLite #23 Param_Set :%s:\n", mt_param_id);  
              #endif  
              strncpy(ap23_param_id, mt_param_id, 16);

              NOW DO VALUE, same layout as the downlink one
              
             return true;           
            }            
            return false;
          case 76:  
            return false;      
        } 
    #endif     
      return false;     
    }


    //==========================   U P L I N K  =========================
    #if defined Support_MavLite
    bool SPort::UnchunkMavLiteUplink() {              // Uplink to FC
      if (mt_seq == 0) {            // first chunk 
        for (int i = 6 ; i <= 8 ; i++, mt_idx++) {
      //    Debug.printf("XX i=%d  mt_idx=%d sp_buff[i]=%x \n",i, mt_idx, sp_buff[i]);
          mt_param_id[mt_idx] = sp_buff[i];
          if (mt_idx >= mt_paylth) {
            return true;
          }        
        }
        return false;
      } else {           // rest of the chunks
        for (int i = 4 ; i <= 8 ; i++, mt_idx++) {
     //     Debug.printf("YY i=%d  mt_idx=%d sp_buff[i]=%x \n",i, mt_idx, sp_buff[i]);
          mt_param_id[mt_idx] = sp_buff[i];
          if (mt_idx >= mt_paylth) {
            mt_param_id[mt_idx] = 0x00; // terminate string      
            return true; 
          }                
         }
       return false;
      }
     return false;
    }
    #endif
    //===================================================================  

    void SPort::SendPassthruPacket(uint8_t sensor_id, uint16_t msg_id) {
      uint8_t *bytes;
  
      if (set.trmode == ground) {         // Only if ground mode send these bytes, else XSR sends them
        SPort::SafeSend(0x7E, false);       //  START/STOP don't stuff or add into crc
        SPort::SafeSend(sensor_id, false);  // alias == instance 
      }
  
      SPort::SafeSend(0x10, true );          //  Passthru - frame_id

      bytes = (uint8_t*)&msg_id;           // cast to bytes

      SPort::SafeSend(bytes[0], true);
      SPort::SafeSend(bytes[1], true);
  
      #if (defined Frs_Debug_Payload) 
        PrintFrPeriod(0);
        Debug.print("\tDataFrame. ID "); 
        PrintByte(bytes[0], 0);
        Debug.print(" "); 
        PrintByte(bytes[1], 0);
      #endif
  
      bytes = (uint8_t*)&fr_payload;       // cast to bytes

      SPort::SafeSend(bytes[0], true);
      SPort::SafeSend(bytes[1], true);
      SPort::SafeSend(bytes[2], true);
      SPort::SafeSend(bytes[3], true);
  
      #if (defined Frs_Debug_Payload) 
        Debug.print("Payload (send order) "); 
        PrintByte(bytes[0], 0);
        Debug.print(" "); 
        PrintByte(bytes[1], 0);
        Debug.print(" "); 
        PrintByte(bytes[2], 0);
        Debug.print(" "); 
        PrintByte(bytes[3], 0);  
        Debug.print("Crc= "); 
        PrintByte(0xFF-CRC, 0);
        Debug.println("/"); 
      #endif 

      SPort::SendCrc();                  

}
    //===================================================================  
    void SPort::SendMavlitePacket(uint8_t sensor_id, uint16_t msg_id) {
      uint8_t *bytes;
  
      if (set.trmode == ground) {         // Only if ground mode send these bytes, else XSR sends them
        SPort::SafeSend(0x7E, false);       //  START/STOP don't stuff or add into crc
        SPort::SafeSend(sensor_id, false);  // alias == instance 
      }
  
      SPort::SafeSend(0x32, false );        //  MavLite frame

      bytes = (uint8_t*)&mt_Payload;      // cast to bytes

      SPort::SafeSend(bytes[0], false);
      SPort::SafeSend(bytes[1], false);
      SPort::SafeSend(bytes[2], false);
      SPort::SafeSend(bytes[3], false);
      SPort::SafeSend(bytes[4], false); 
      SPort::SafeSend(bytes[5], false);

  
      #if (defined Frs_Debug_Payload) || (defined Debug_MavLite)
        Debug.print("Payload (send order) "); 
        PrintByte(bytes[0], 0);
        Debug.print(" "); 
        PrintByte(bytes[1], 0);
        Debug.print(" "); 
        PrintByte(bytes[2], 0);
        Debug.print(" "); 
        PrintByte(bytes[3], 0);  
        PrintByte(bytes[4], 0);   
        PrintByte(bytes[5], 0);  
        Debug.println("/"); 
      #endif
  
    }
    //===================================================================   
    void SPort::SendCrc() {
      uint8_t byte;
      byte = 0xFF-CRC;

      SPort::SafeSend(byte, false);
 
      // PrintByte(byte, 1);
      // Debug.println("");
      CRC = 0;          // CRC reset
    }
    //===================================================================  

    uint16_t SPort::FirstEmptyMt20Row() {
      uint16_t i = 0;
      while (mt20[i].inuse == 1) {   // find empty mt20 uplink row
      //  Debug.printf("i=%d  mt20[i].inuse=%d\n", i, mt20[i].inuse);

        if (millis() - mt20[i].millis > 5000) { // expire the row if no reply from FC in 5 seconds ? bad param_id
          mt20[i].inuse = 0;
          Debug.printf("param_id %s not received back from FC. Timed out.\n", mt20[i].param_id);
        }
        i++; 
        if ( i >= mt20_rows-1) {
          mt20_buf_full_gear++;
          if ( (mt20_buf_full_gear == 0) || (mt20_buf_full_gear%1000 == 0)) {
            Debug.println("mt20 uplink buffer full");  // Report every so often
          }
          return 0xffff;
        }
      }
      return i;
    }    
    //=================================================================== 
    void SPort::PushMessage(uint16_t msg_id, uint8_t sub_id) {
      switch(msg_id) {
        case 0x16:                   // msg_id 0x16 MavLite PARAM_VALUE ( #22 )
          SPort::Push_Param_Val_016(msg_id);
          break; 
      
        case 0x800:                  // msg_id 0x800 Lat & Lon
          if (sub_id == 0) {
            SPort::Push_Lat_800(msg_id);
          }
          if (sub_id == 1) {
            SPort::Push_Lon_800(msg_id);
          }
          break;            
        case 0x5000:                 // msg_id 0x5000 Status Text            
            SPort::Push_Text_Chunks_5000(msg_id);
            break;
        
        case 0x5001:                // msg_id 0x5001 AP Status
          SPort::Push_AP_status_5001(msg_id);
          break; 

        case 0x5002:                // msg_id 0x5002 GPS Status
          SPort::Push_GPS_status_5002(msg_id);
          break; 
          
        case 0x5003:                //msg_id 0x5003 Batt 1
          SPort::Push_Bat1_5003(msg_id);
          break; 
                    
        case 0x5004:                // msg_id 0x5004 Home
          SPort::Push_Home_5004(msg_id);
          break; 

        case 0x5005:                // msg_id 0x5005 Velocity and yaw
          SPort::Push_VelYaw_5005(msg_id);
          break; 

        case 0x5006:                // msg_id 0x5006 Attitude and range
          SPort::Push_Atti_5006(msg_id);
          break; 
      
        case 0x5007:                // msg_id 0x5007 Parameters 
          SPort::Push_Parameters_5007(msg_id, sub_id);
          break; 
      
        case 0x5008:                // msg_id 0x5008 Batt 2
          SPort::Push_Bat2_5008(msg_id);
          break; 

        case 0x5009:                // msg_id 0x5009 Waypoints/Missions 
          SPort::Push_WayPoint_5009(msg_id);
          break;       

        case 0x50F1:                // msg_id 0x50F1 Servo_Raw            
          SPort::Push_Servo_Raw_50F1(msg_id);
          break;      

        case 0x50F2:                // msg_id 0x50F2 VFR HUD          
          SPort::Push_VFR_Hud_50F2(msg_id);
          break;    

        case 0x50F3:                // msg_id 0x50F3 Wind Estimate      
       //   SPort::Push_Wind_Estimate_50F3(msg_id);  // not presently implemented
          break; 
        case 0xF101:                // msg_id 0xF101 RSSI      
          SPort::Push_Rssi_F101(msg_id);      
          break;       
        default:
          Debug.print("Warning, msg_id "); Debug.print(msg_id, HEX); Debug.println(" unknown");
          break;       
      }            
    }
    //=================================================================== 
    void SPort::PushToEmptyRow(uint16_t msg_id, uint8_t sub_id) {
      sb_row = 0;
      while (sb[sb_row].inuse) {   // find empty s.port row 
        sb_row++; 
        if (sb_row >= sb_rows-1) {

          if ( (sb_buf_full_gear == 0) || (sb_buf_full_gear%4000 == 0)) {
            Debug.println("S.Port scheduler buffer full. Check S.Port downlink");  // Report every so often
          }
          sb_buf_full_gear++;
          return;     
        }
      }
      sb_unsent++;

      // The push
      sb[sb_row].millis = millis();
      sb[sb_row].inuse = 1;                // 0=free for use, 1=occupied
      sb[sb_row].msg_id = msg_id;
      sb[sb_row].sub_id = sub_id;
  
      if(SPort::msg_class_now(msg_id) == passthru) {
          sb[sb_row].payload.passthru = fr_payload;  
      } else
      if(SPort::msg_class_now(msg_id)== mavlite) {
          sb[sb_row].payload.mavlite = mt_Payload;  
      }
  
      #if (defined Frs_Debug_Scheduler) || (defined MavLite_Debug_Scheduler) 
  
        uint16_t msgid_filter;  
        #if (defined MavLite_Debug_Scheduler)
          msgid_filter = 0x100;
        #else
          msgid_filter = 0xffff; // high values
        #endif
    
        if (msg_id < msgid_filter) {
          Debug.print(sb_unsent); 
          Debug.printf("\tPush row= %3d", sb_row );
          Debug.print("  msg_id=0x"); Debug.print(msg_id, HEX);
          if (msg_id < 0x1000) Debug.print(" ");
          Debug.printf("  sub_id= %2d", sub_id);
          pb_rx=false;
          SPort::PrintPayload(SPort::msg_class_now(msg_id));
          Debug.println();      
        }
      #endif
    }
    //=================================================================== 
 
    void SPort::Push_Param_Val_016(uint16_t msg_id) {   //  0x16 MavLite PARAM_VALUE ( #22 )
    #if defined Support_MavLite  
      printcrc = 1; //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 
      mliteMsg.msg_id = msg_id;                      // 0x16;  
      mt22_lth = 4 +strlen(ap22_param_id);             // payload = value + param 
      Debug.printf("mt22_lth+4=%d\n", mt22_lth);        //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
      mliteMsg.lth = mt22_lth;                        // Payload lth = 17
      mliteMsg.value = ap22_param_value;
      strncpy(mliteMsg.param_id, ap22_param_id, 16);
      mt22_tot = 4 + mliteMsg.lth;            // msg_id + lth + padd + padd + payload
 
      #if defined Debug_MavLite
        Debug.printf("PackMavLiteParamValue16 msg_id=0x%X lth=%d param_value=%.3f param_id=%s \n"
          , mliteMsg.msg_id, mliteMsg.lth, mliteMsg.value, mliteMsg.param_id );    
          
        Debug.print("mliteMsg=");
        for (int i = 0 ; i < mt22_tot ; i++) {
         if ((i == 2) || !((i+2)%6)) Debug.print("|");
         PrintByteNon(mliteMsg.raw[i]);
        }

        Debug.print("|\t|");
        for (int i = 0 ; i < mt22_tot ; i++) {
         if ((mliteMsg.raw[i] >31) && (mliteMsg.raw[i]<127)) Debug.write(mliteMsg.raw[i]);
          }
        Debug.println("|");
      #endif

      mt_chunk = 0; 
      mtmsg_idx = 4;   // 0 thru 3 skipped

  
      while (mtmsg_idx <= mt22_tot) {    
        Debug.printf("mtmsg_idx=%d  mt22_tot=%d   mt_chunk=%d\n", mtmsg_idx, mt22_tot, mt_chunk     ); 
        if (mt_chunk == 0) {

          //======= message to frame (chunk)
          // payload starting position  msgid, lth - then 20B payload, then Crc
    
          // first chunk 
          mt_Payload.seq = mt_chunk;      SPort::AddInCrc(mt_Payload.lth);
          mt_Payload.lth = mliteMsg.lth;  SPort::AddInCrc(mt_Payload.lth);     // idx = 1          
          mt_Payload.msg_id = msg_id;     SPort::AddInCrc(mt_Payload.msg_id);  // idx = 0

         //______________________________________________________
         for (int i = 3 ; i < 6 ; i++) {  
            mt_Payload.raw[i] = mliteMsg.raw[mtmsg_idx];  
            SPort::AddInCrc(mt_Payload.raw[i]); 
     /*
            Debug.printf("i=%d mtmsg_idx=%d mt_Payload.raw[i]=0x%X\t|", i, mtmsg_idx, mt_Payload.raw[i]); 
            if ((mt_Payload.raw[i] > 31) && (mt_Payload.raw[i] < 127)) 
              Debug.write(mt_Payload.raw[i]);
              else Debug.print(" ");
            Debug.println("|");  
    */
        
            mtmsg_idx++;      
          } 
      
          //_______________________________________________________
          // rest of the frames / chunks
        } else {           
          mt_Payload.seq = mt_chunk;  SPort::AddInCrc(mt_Payload.seq); 
          //_______________________________________________________
          for (int i = 1 ; i < 6 ; i++) { 
          
            if (mtmsg_idx >= mt22_tot) {  // break out of the for loop
              mt_Payload.raw[i] = CRC;     // no 1s complement? and append the CRC
      //       Debug.printf("mtmsg_idx=%d  mliteMsg.raw[mtmsg_idx]=0x%X\n", mtmsg_idx, mliteMsg.raw[mtmsg_idx] );
              mtmsg_idx++;  // gets me out of the while loopq/
              break; // short chunk  - break out of for loop
            }

            mt_Payload.raw[i] = mliteMsg.raw[mtmsg_idx];
            SPort::AddInCrc(mt_Payload.raw[i]); 
     /*       
            Debug.printf("i=%d mtmsg_idx=%d mt_Payload.raw[i]=0x%X\t|", i, mtmsg_idx, mt_Payload.raw[i]); 
            if ((mt_Payload.raw[i] > 31) && (mt_Payload.raw[i] < 127)) 
              Debug.write(mt_Payload.raw[i]);
              else Debug.print(" ");
            Debug.println("|");
    */        
            mt_Payload.raw[i] = mliteMsg.raw[mtmsg_idx];   
            mtmsg_idx++;                                    
          }  // end of for
          //_________________________________________________________ 
        }   // end of else - chunks > 0

        SPort::PushToEmptyRow(msg_id, 0);  

        #if defined Frs_Debug_All || defined Debug_MavLite
          Debug.printf("MavLite downlink Param_Value chunk 0x%x: ", msg_id);
          Debug.printf(" seq=%d  ", mt_Payload.seq);
          pb_rx = false;
          SPort::PrintPayload(mavlite); 
          Debug.println();
        #endif
      
        mt_chunk++;
        mt_Payload = {};  // clear payload struct
      } // end of while loop

      CRC = 0;
       printcrc = 0;
    #endif    
    }

    //===================================================================   
    void SPort::Push_Lat_800(uint16_t msg_id) {       // 0x800
      fr_gps_status = ap_fixtype < 3 ? ap_fixtype : 3;                   //  0 - 3 
      if (fr_gps_status < 3) return;
      if ((px4_flight_stack) || (pitlab_flight_stack)) {
        fr_lat = Abs(ap_lat24) / 100 * 6;  // ap_lat * 60 / 1000
        if (ap_lat24<0) 
          ms2bits = 1;
        else ms2bits = 0;    
      } else {
        fr_lat = Abs(ap_lat33) / 100 * 6;  // ap_lat * 60 / 1000
        if (ap_lat33<0) 
          ms2bits = 1;
        else ms2bits = 0;
      }
      fr_payload = 0;
      bit32Pack(fr_lat, 0, 30);
      bit32Pack(ms2bits, 30, 2);
          
      #if defined Frs_Debug_All || defined Frs_Debug_LatLon
        PrintFrPeriod(0); 
        Debug.print("Passthru out LatLon 0x800: ");
        Debug.print(" ap_lat33="); Debug.print((float)ap_lat33 / 1E7, 7); 
        Debug.print(" fr_lat="); Debug.print(fr_lat);  
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
       SPort::PrintPayload(passthru);
        int32_t r_lat = (bit32Unpack(fr_payload,0,30) * 100 / 6);
        Debug.print(" lat unpacked="); Debug.println(r_lat );    
      #endif

     SPort::PushToEmptyRow(msg_id, 0);        
    }
    //===================================================================  
    void SPort::Push_Lon_800(uint16_t msg_id) {      // 0x800
      fr_gps_status = ap_fixtype < 3 ? ap_fixtype : 3;                   //  0 - 3 
      if (fr_gps_status < 3) return;
      if ((px4_flight_stack) || (pitlab_flight_stack)) {
        fr_lon = Abs(ap_lon24) / 100 * 6;  // ap_lon * 60 / 1000
        if (ap_lon24<0) {
          ms2bits = 3;
        }
        else {
          ms2bits = 2;    
        }
      } else {
        fr_lon = Abs(ap_lon33) / 100 * 6;  // ap_lon * 60 / 1000
        if (ap_lon33<0) { 
          ms2bits = 3;
        }
        else {
          ms2bits = 2;
        }
      }
      fr_payload = 0;
      bit32Pack(fr_lon, 0, 30);
      bit32Pack(ms2bits, 30, 2);
          
      #if defined Frs_Debug_All || defined Frs_Debug_LatLon
       PrintFrPeriod(0); 
        Debug.print("Passthru out LatLon 0x800: ");  
        Debug.print(" ap_lon33="); Debug.print((float)ap_lon33 / 1E7, 7);     
        Debug.print(" fr_lon="); Debug.print(fr_lon); 
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
        SPort::PrintPayload(passthru);
        int32_t r_lon = (bit32Unpack(fr_payload,0,30) * 100 / 6);
        Debug.print(" lon unpacked="); Debug.println(r_lon );  
      #endif

       SPort::PushToEmptyRow(msg_id, 1); 
    }
    //===================================================================  
    void SPort::Push_Text_Chunks_5000(uint16_t msg_id) {

      // status text  char[50] no null,  ap-text char[60]

      for (int i=0; i<50 ; i++) {       // Get text len
        if (ap_text[i]==0) {            // end of text
          len=i;
          break;
        }
      }
  
      ap_text[len+1]=0x00;
      ap_text[len+2]=0x00;  // mark the end of text chunk +
      ap_text[len+3]=0x00;
      ap_text[len+4]=0x00;
          
      ap_txtlth = len;
  
      // look for simple-mode status change messages       
      if (strcmp (ap_text,"SIMPLE mode on") == 0)
        ap_simple = true;
      else if (strcmp (ap_text,"SIMPLE mode off") == 0)
        ap_simple = false;

      fr_severity = ap_severity;
      fr_txtlth = ap_txtlth;
      memcpy(fr_text, ap_text, fr_txtlth+4);   // plus rest of last chunk at least
      fr_simple = ap_simple;

      #if defined Frs_Debug_All || defined Frs_Debug_StatusText
        PrintFrPeriod(0); 
        Debug.print("Passthru out AP_Text 0x5000: ");  
        Debug.print(" fr_severity="); Debug.print(fr_severity);
        Debug.print(" "); Debug.print(MavSeverity(fr_severity)); 
        Debug.print(" Text= ");  Debug.print(" |"); Debug.print(fr_text); Debug.println("| ");
      #endif

      fr_chunk_idx = 0;

      while (fr_chunk_idx <= (fr_txtlth)) {                 // send multiple 4 byte (32b) chunks
    
        fr_chunk_num = (fr_chunk_idx / 4) + 1;
    
        fr_chunk[0] = fr_text[fr_chunk_idx];
        fr_chunk[1] = fr_text[fr_chunk_idx+1];
        fr_chunk[2] = fr_text[fr_chunk_idx+2];
        fr_chunk[3] = fr_text[fr_chunk_idx+3];
    
        fr_payload = 0;
        bit32Pack(fr_chunk[0], 24, 7);
        bit32Pack(fr_chunk[1], 16, 7);
        bit32Pack(fr_chunk[2], 8, 7);    
        bit32Pack(fr_chunk[3], 0, 7);  
    
        #if defined Frs_Debug_All || defined Frs_Debug_StatusText
          PrintFrPeriod(0); 
          Debug.print(" fr_chunk_num="); Debug.print(fr_chunk_num); 
          Debug.print(" fr_txtlth="); Debug.print(fr_txtlth); 
          Debug.print(" fr_chunk_idx="); Debug.print(fr_chunk_idx); 
          Debug.print(" "); 
          strncpy(fr_chunk_print,fr_chunk, 4);
          fr_chunk_print[4] = 0x00;
          Debug.print(" |"); Debug.print(fr_chunk_print); Debug.print("| ");
          Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
          SPort::PrintPayload(passthru);
          Debug.println();
        #endif  

        if (fr_chunk_idx+4 > (fr_txtlth)) {

          bit32Pack((fr_severity & 0x1), 7, 1);            // ls bit of severity
          bit32Pack(((fr_severity & 0x2) >> 1), 15, 1);    // mid bit of severity
          bit32Pack(((fr_severity & 0x4) >> 2) , 23, 1);   // ms bit of severity                
          bit32Pack(0, 31, 1);     // filler
      
          #if defined Frs_Debug_All || defined Frs_Debug_StatusText
            PrintFrPeriod(0); 
            Debug.print(" fr_chunk_num="); Debug.print(fr_chunk_num); 
            Debug.print(" fr_severity="); Debug.print(fr_severity);
            Debug.print(" "); Debug.print(MavSeverity(fr_severity)); 
            bool lsb = (fr_severity & 0x1);
            bool sb = (fr_severity & 0x2) >> 1;
            bool msb = (fr_severity & 0x4) >> 2;
            Debug.print(" ls bit="); Debug.print(lsb); 
            Debug.print(" mid bit="); Debug.print(sb); 
            Debug.print(" ms bit="); Debug.print(msb); 
            Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
            SPort::PrintPayload(passthru);
            Debug.println(); Debug.println();
         #endif 
         }

       SPort::PushToEmptyRow(msg_id, fr_chunk_num); 

       #if defined Send_status_Text_3_Times 
        SPort::PushToEmptyRow(msg_id, fr_chunk_num); 
        SPort::PushToEmptyRow(msg_id, fr_chunk_num); 
       #endif 
    
       fr_chunk_idx +=4;
     }
  
      fr_chunk_idx = 0;
   
    }

    //===================================================================   
    void SPort::Push_AP_status_5001(uint16_t msg_id) {
      if (ap_type == 6) return;      // If GCS heartbeat ignore it  -  yaapu  - ejs also handled at #0 read
      fr_payload = 0;
     // fr_simple = ap_simple;         // Derived from "ALR SIMPLE mode on/off" text messages
      fr_simple = 0;                   // stops repeated 'simple mode enabled' and flight mode messages
      fr_armed = ap_base_mode >> 7;  
      fr_land_complete = fr_armed;
  
      if (px4_flight_stack) {
        fr_flight_mode = PX4FlightModeNum(px4_main_mode, px4_sub_mode);
      } else 
      if (pitlab_flight_stack) {
        fr_flight_mode = ap_custom_mode;
      } else { //  APM Flight Stack
        fr_flight_mode = ap_custom_mode + 1; // AP_CONTROL_MODE_LIMIT - ls 5 bits
      }
  
      fr_imu_temp = ap26_temp;
    
      bit32Pack(fr_flight_mode, 0, 5);      // Flight mode   0-32 - 5 bits
      bit32Pack(fr_simple ,5, 2);           // Simple/super simple mode flags
      bit32Pack(fr_land_complete ,7, 1);    // Landed flag
      bit32Pack(fr_armed ,8, 1);            // Armed
      bit32Pack(fr_bat_fs ,9, 1);           // Battery failsafe flag
      bit32Pack(fr_ekf_fs ,10, 2);          // EKF failsafe flag
      bit32Pack(px4_flight_stack ,12, 1);   // px4_flight_stack flag
      bit32Pack(fr_imu_temp, 26, 6);        // imu temperature in cdegC

      #if defined Frs_Debug_All || defined Frs_Debug_APStatus
        PrintFrPeriod(0); 
        Debug.print("Passthru out AP_status 0x5001: ");   
        Debug.print(" fr_flight_mode="); Debug.print(fr_flight_mode);
        Debug.print(" fr_simple="); Debug.print(fr_simple);
        Debug.print(" fr_land_complete="); Debug.print(fr_land_complete);
        Debug.print(" fr_armed="); Debug.print(fr_armed);
        Debug.print(" fr_bat_fs="); Debug.print(fr_bat_fs);
        Debug.print(" fr_ekf_fs="); Debug.print(fr_ekf_fs);
        Debug.print(" px4_flight_stack="); Debug.print(px4_flight_stack);
        Debug.print(" fr_imu_temp="); Debug.print(fr_imu_temp);
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
        SPort::PrintPayload(passthru);
        Debug.println();
      #endif

      SPort::PushToEmptyRow(msg_id, 0);       

    }
    //===================================================================   
    void SPort::Push_GPS_status_5002(uint16_t msg_id) {
      fr_payload = 0;
      if (ap_sat_visible > 15)
        fr_numsats = 15;
      else
        fr_numsats = ap_sat_visible;
  
      bit32Pack(fr_numsats ,0, 4); 
          
      fr_gps_status = ap_fixtype < 3 ? ap_fixtype : 3;                   //  0 - 3
      fr_gps_adv_status = ap_fixtype > 3 ? ap_fixtype - 3 : 0;           //  4 - 8 -> 0 - 3   
          
      fr_amsl = ap_amsl24 / 100;  // dm
      fr_hdop = ap_eph /10;
          
      bit32Pack(fr_gps_status ,4, 2);       // part a, 3 bits
      bit32Pack(fr_gps_adv_status ,14, 2);  // part b, 3 bits
          
      #if defined Frs_Debug_All || defined Frs_Debug_GPS_status
        PrintFrPeriod(0); 
        Debug.print("Passthru out GPS Status 0x5002: ");   
        Debug.print(" fr_numsats="); Debug.print(fr_numsats);
        Debug.print(" fr_gps_status="); Debug.print(fr_gps_status);
        Debug.print(" fr_gps_adv_status="); Debug.print(fr_gps_adv_status);
        Debug.print(" fr_amsl="); Debug.print(fr_amsl);
        Debug.print(" fr_hdop="); Debug.print(fr_hdop);
      #endif
          
      fr_amsl = prep_number(fr_amsl,2,2);                       // Must include exponent and mantissa    
      fr_hdop = prep_number(fr_hdop,2,1);
          
      #if defined Frs_Debug_All || defined Frs_Debug_GPS_status
        Debug.print(" After prep: fr_amsl="); Debug.print(fr_amsl);
        Debug.print(" fr_hdop="); Debug.print(fr_hdop); 
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
        SPort::PrintPayload(passthru);
        Debug.println(); 
      #endif     
              
      bit32Pack(fr_hdop ,6, 8);
      bit32Pack(fr_amsl ,22, 9);
      bit32Pack(0, 31,0);  // 1=negative 

      SPort::PushToEmptyRow(msg_id, 0);  
    }
    //===================================================================   
    void SPort::Push_Bat1_5003(uint16_t msg_id) {   //  Into S.Port table from #1 SYS_status only
      fr_payload = 0;
      fr_bat1_volts = ap_voltage_battery1 / 100;         // Were mV, now dV  - V * 10
      fr_bat1_amps = ap_current_battery1 ;               // Remain       dA  - A * 10   
  
      // fr_bat1_mAh is populated at #147 depending on battery id.  Into S.Port table from #1 SYS_status only.
      //fr_bat1_mAh = Total_mAh1();  // If record type #147 is not sent and good
  
      #if defined Frs_Debug_All || defined Debug_Batteries
        PrintFrPeriod(0); 
        Debug.print("Passthru out Bat1 0x5003: ");   
        Debug.print(" fr_bat1_volts="); Debug.print(fr_bat1_volts);
        Debug.print(" fr_bat1_amps="); Debug.print(fr_bat1_amps);
        Debug.print(" fr_bat1_mAh="); Debug.print(fr_bat1_mAh);
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
        SPort::PrintPayload(passthru);
        Debug.println();               
      #endif
          
      bit32Pack(fr_bat1_volts ,0, 9);
      fr_bat1_amps = prep_number(roundf(fr_bat1_amps * 0.1F),2,1);          
      bit32Pack(fr_bat1_amps,9, 8);
      bit32Pack(fr_bat1_mAh,17, 15);

      SPort::PushToEmptyRow(msg_id, 0);  
                       
    }
    //===================================================================
       
    void SPort::Push_Home_5004(uint16_t msg_id) {
      fr_payload = 0;
    
      lon1=hom.lon/180*PI;  // degrees to radians
      lat1=hom.lat/180*PI;
      lon2=cur.lon/180*PI;
      lat2=cur.lat/180*PI;

      //Calculate azimuth bearing of craft from home
      a=atan2(sin(lon2-lon1)*cos(lat2), cos(lat1)*sin(lat2)-sin(lat1)*cos(lat2)*cos(lon2-lon1));
      az=a*180/PI;  // radians to degrees
      if (az<0) az=360+az;

      fr_home_angle = Add360(az, -180);                           // Is now the angle from the craft to home in degrees
  
      fr_home_arrow = fr_home_angle * 0.3333;                     // Units of 3 degrees

      // Calculate the distance from home to craft
      dLat = (lat2-lat1);
      dLon = (lon2-lon1);
      a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(lat1) * cos(lat2); 
      c = 2* asin(sqrt(a));    // proportion of Earth's radius
      dis = 6371000 * c;       // radius of the Earth is 6371km

      if (homGood)
        fr_home_dist = (int)dis;
      else
        fr_home_dist = 0;

        fr_home_alt = ap_alt_ag / 100;    // mm->dm
        
      #if defined Frs_Debug_All || defined Frs_Debug_Home
        PrintFrPeriod(0); 
        Debug.print("Passthru out Home 0x5004: ");         
        Debug.print("fr_home_dist=");  Debug.print(fr_home_dist);
        Debug.print(" fr_home_alt=");  Debug.print(fr_home_alt);
        Debug.print(" az=");  Debug.print(az);
        Debug.print(" fr_home_angle="); Debug.print(fr_home_angle);  
        Debug.print(" fr_home_arrow="); Debug.print(fr_home_arrow);         // units of 3 deg  
        Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
        SPort::PrintPayload(passthru);
        Debug.println();      
      #endif
      fr_home_dist = prep_number(roundf(fr_home_dist), 3, 2);
      bit32Pack(fr_home_dist ,0, 12);
      fr_home_alt = prep_number(roundf(fr_home_alt), 3, 2);
      bit32Pack(fr_home_alt ,12, 12);
      if (fr_home_alt < 0)
        bit32Pack(1,24, 1);
      else  
        bit32Pack(0,24, 1);
      bit32Pack(fr_home_arrow,25, 7);

      SPort::PushToEmptyRow(msg_id, 0);  

    }

    //===================================================================   
    
    void SPort::Push_VelYaw_5005(uint16_t msg_id) {
      fr_payload = 0;
  
      fr_vy = ap_hud_climb * 10;   // from #74   m/s to dm/s;
      fr_vx = ap_hud_grd_spd * 10;  // from #74  m/s to dm/s

      //fr_yaw = (float)ap_gps_hdg / 10;  // (degrees*100) -> (degrees*10)
      fr_yaw = ap_hud_hdg * 10;              // degrees -> (degrees*10)
  
      #if defined Frs_Debug_All || defined Frs_Debug_VelYaw
        PrintFrPeriod(0); 
        Debug.print("Passthru out VelYaw 0x5005:");  
        Debug.print(" fr_vy=");  Debug.print(fr_vy);       
        Debug.print(" fr_vx=");  Debug.print(fr_vx);
        Debug.print(" fr_yaw="); Debug.print(fr_yaw);
     
      #endif
      if (fr_vy<0)
        bit32Pack(1, 8, 1);
      else
        bit32Pack(0, 8, 1);
      fr_vy = prep_number(roundf(fr_vy), 2, 1);  // Vertical velocity
      bit32Pack(fr_vy, 0, 8);   

      fr_vx = prep_number(roundf(fr_vx), 2, 1);  // Horizontal velocity
      bit32Pack(fr_vx, 9, 8);    
      fr_yaw = fr_yaw * 0.5f;                   // Unit = 0.2 deg
      bit32Pack(fr_yaw ,17, 11);  

     #if defined Frs_Debug_All || defined Frs_Debug_VelYaw
       Debug.print(" After prep:"); \
       Debug.print(" fr_vy=");  Debug.print((int)fr_vy);          
       Debug.print(" fr_vx=");  Debug.print((int)fr_vx);  
       Debug.print(" fr_yaw="); Debug.print((int)fr_yaw);  
       Debug.print(" fr_payload="); Debug.print(fr_payload); Debug.print(" ");
       SPort::PrintPayload(passthru);
       Debug.println();                 
     #endif

     SPort::PushToEmptyRow(msg_id, 0);  
    
    }
    //=================================================================== 
      
    void SPort::Push_Atti_5006(uint16_t msg_id) {
      fr_payload = 0;
  
      fr_roll = (ap_roll * 5) + 900;             //  -- fr_roll units = [0,1800] ==> [-180,180]
      fr_pitch = (ap_pitch * 5) + 450;           //  -- fr_pitch units = [0,900] ==> [-90,90]
      fr_range = roundf(ap_range*100);   
      bit32Pack(fr_roll, 0, 11);
      bit32Pack(fr_pitch, 11, 10); 
      bit32Pack(prep_number(fr_range,3,1), 21, 11);
      #if defined Frs_Debug_All || defined Frs_Debug_AttiRange
        PrintFrPeriod(0); 
        Debug.print("Passthru out Attitude 0x5006: ");         
        Debug.print("fr_roll=");  Debug.print(fr_roll);
        Debug.print(" fr_pitch=");  Debug.print(fr_pitch);
        Debug.print(" fr_range="); Debug.print(fr_range);
        Debug.print(" Payload="); Debug.println(fr_payload);  
      #endif

      SPort::PushToEmptyRow(msg_id, 0);   
     
    }
    //===================================================================   
    
    void SPort::Push_Parameters_5007(uint16_t msg_id, uint8_t sub_id) {
    
      switch(sub_id) {
        case 1:                                    // Frame type
          fr_param_id = 1;
          fr_frame_type = ap_type;
      
          fr_payload = 0;
          bit32Pack(fr_frame_type, 0, 24);
          bit32Pack(fr_param_id, 24, 4);

          #if defined Frs_Debug_All || defined Frs_Debug_Params
            PrintFrPeriod(0);  
            Debug.print("Passthru out Params 0x5007: ");   
            Debug.print(" fr_param_id="); Debug.print(fr_param_id);
            Debug.print(" fr_frame_type="); Debug.print(fr_frame_type);  
            Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
            SPort::PrintPayload(passthru);
            Debug.println();                
          #endif
      
          SPort::PushToEmptyRow(msg_id, sub_id);
          break;    
       
        case 4:    // Battery pack 1 capacity
          fr_param_id = sub_id;
          #if (Battery_mAh_Source == 2)    // Local
            fr_bat1_capacity = bat1_capacity;
          #elif  (Battery_mAh_Source == 1) //  FC
            fr_bat1_capacity = ap_bat1_capacity;
          #endif 

          fr_payload = 0;
          bit32Pack(fr_bat1_capacity, 0, 24);
          bit32Pack(fr_param_id, 24, 4);

          #if defined Frs_Debug_All || defined Frs_Debug_Params || defined Debug_Batteries
            PrintFrPeriod(0);       
            Debug.print("Passthru out Params 0x5007: ");   
            Debug.print(" fr_param_id="); Debug.print(fr_param_id);
            Debug.print(" fr_bat1_capacity="); Debug.print(fr_bat1_capacity);  
            Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
            SPort::PrintPayload(passthru);
            Debug.println();                   
          #endif

          SPort::PushToEmptyRow(msg_id, sub_id); 
          break; 
      
        case 5:                 // Battery pack 2 capacity
          fr_param_id = sub_id;
          #if (Battery_mAh_Source == 2)    // Local
            fr_bat2_capacity = bat2_capacity;
          #elif  (Battery_mAh_Source == 1) //  FC
            fr_bat2_capacity = ap_bat2_capacity;
          #endif  

          fr_payload = 0;
          bit32Pack(fr_bat2_capacity, 0, 24);
          bit32Pack(fr_param_id, 24, 4);
      
          #if defined Frs_Debug_All || defined Frs_Debug_Params || defined Debug_Batteries
            PrintFrPeriod(0);  
            Debug.print("Passthru out Params 0x5007: ");   
            Debug.print(" fr_param_id="); Debug.print(fr_param_id);
            Debug.print(" fr_bat2_capacity="); Debug.print(fr_bat2_capacity); 
            Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
            SPort::PrintPayload(passthru);
            Debug.println();           
          #endif
      
          SPort::PushToEmptyRow(msg_id, sub_id);
          break; 
    
         case 6:               // Number of waypoints in mission                       
          fr_param_id = sub_id;
          fr_mission_count = ap_mission_count;

          fr_payload = 0;
          bit32Pack(fr_mission_count, 0, 24);
          bit32Pack(fr_param_id, 24, 4);

          SPort::PushToEmptyRow(msg_id, sub_id);        
      
          #if defined Frs_Debug_All || defined Frs_Debug_Params || defined Debug_Batteries
            PrintFrPeriod(0); 
            Debug.print("Passthru out Params 0x5007: ");   
            Debug.print(" fr_param_id="); Debug.print(fr_param_id);
            Debug.print(" fr_mission_count="); Debug.println(fr_mission_count);           
          #endif
 
          fr_paramsSent = true;          // get this done early on and then regularly thereafter

          break;
      }    
    }
    //===================================================================   
    
    void SPort::Push_Bat2_5008(uint16_t msg_id) {
       fr_payload = 0;
   
       fr_bat2_volts = ap_voltage_battery2 / 100;         // Were mV, now dV  - V * 10
       fr_bat2_amps = ap_current_battery2 ;               // Remain       dA  - A * 10   
   
      // fr_bat2_mAh is populated at #147 depending on battery id
      //fr_bat2_mAh = Total_mAh2();  // If record type #147 is not sent and good
  
      #if defined Frs_Debug_All || defined Debug_Batteries
        PrintFrPeriod(0);  
        Debug.print("Passthru out Bat2 0x5008: ");   
        Debug.print(" fr_bat2_volts="); Debug.print(fr_bat2_volts);
        Debug.print(" fr_bat2_amps="); Debug.print(fr_bat2_amps);
        Debug.print(" fr_bat2_mAh="); Debug.print(fr_bat2_mAh);
        Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
        SPort::PrintPayload(passthru);
        Debug.println();                  
      #endif        
          
      bit32Pack(fr_bat2_volts ,0, 9);
      fr_bat2_amps = prep_number(roundf(fr_bat2_amps * 0.1F),2,1);          
      bit32Pack(fr_bat2_amps,9, 8);
      bit32Pack(fr_bat2_mAh,17, 15);      

      SPort::PushToEmptyRow(msg_id, 1);           
    }


    //===================================================================   

    void SPort::Push_WayPoint_5009(uint16_t msg_id) {
      fr_payload = 0;
  
      fr_ms_seq = ap_ms_seq;                                      // Current WP seq number, wp[0] = wp1, from regular #42
  
      fr_ms_dist = ap_wp_dist;                                        // Distance to next WP  

      fr_ms_xtrack = ap_xtrack_error;                                 // Cross track error in metres from #62
      fr_ms_target_bearing = ap_target_bearing;                       // Direction of next WP
      fr_ms_cog = ap_cog * 0.01;                                      // COG in degrees from #24
      int32_t angle = (int32_t)wrap_360(fr_ms_target_bearing - fr_ms_cog);
      int32_t arrowStep = 360 / 8; 
      fr_ms_offset = ((angle + (arrowStep/2)) / arrowStep) % 8;       // Next WP bearing offset from COG

      /*
   
    0 - up
    1 - up-right
    2 - right
    3 - down-right
    4 - down
    5 - down - left
    6 - left
    7 - up - left
 
       */
      #if defined Frs_Debug_All || defined Frs_Debug_Mission
        PrintFrPeriod(0);  
        Debug.print("Passthru out RC 0x5009: ");   
        Debug.print(" fr_ms_seq="); Debug.print(fr_ms_seq);
        Debug.print(" fr_ms_dist="); Debug.print(fr_ms_dist);
        Debug.print(" fr_ms_xtrack="); Debug.print(fr_ms_xtrack, 3);
        Debug.print(" fr_ms_target_bearing="); Debug.print(fr_ms_target_bearing, 0);
        Debug.print(" fr_ms_cog="); Debug.print(fr_ms_cog, 0);  
        Debug.print(" fr_ms_offset="); Debug.print(fr_ms_offset);
        Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
        SPort::PrintPayload(passthru);         
        Debug.println();      
      #endif

      bit32Pack(fr_ms_seq, 0, 10);    //  WP number

      fr_ms_dist = prep_number(roundf(fr_ms_dist), 3, 2);       //  number, digits, power
      bit32Pack(fr_ms_dist, 10, 12);    

      fr_ms_xtrack = prep_number(roundf(fr_ms_xtrack), 1, 1);  
      bit32Pack(fr_ms_xtrack, 22, 6); 

      bit32Pack(fr_ms_offset, 29, 3);  

      SPort::PushToEmptyRow(msg_id, 1);  
        
    }

    //===================================================================  
    
    void SPort::Push_Servo_Raw_50F1(uint16_t msg_id) {
    uint8_t sv_chcnt = 8;
      fr_payload = 0;
  
      if (sv_count+4 > sv_chcnt) { // 4 channels at a time
        sv_count = 0;
        return;
      } 

      uint8_t  chunk = sv_count / 4; 

      fr_sv[1] = PWM_To_63(ap_chan_raw[sv_count]);     // PWM 1000 to 2000 -> 6bit 0 to 63
      fr_sv[2] = PWM_To_63(ap_chan_raw[sv_count+1]);    
      fr_sv[3] = PWM_To_63(ap_chan_raw[sv_count+2]); 
      fr_sv[4] = PWM_To_63(ap_chan_raw[sv_count+3]); 

      bit32Pack(chunk, 0, 4);                // chunk number, 0 = chans 1-4, 1=chans 5-8, 2 = chans 9-12, 3 = chans 13 -16 .....
      bit32Pack(Abs(fr_sv[1]) ,4, 6);        // fragment 1 
      if (fr_sv[1] < 0)
        bit32Pack(1, 10, 1);                 // neg
      else 
        bit32Pack(0, 10, 1);                 // pos          
      bit32Pack(Abs(fr_sv[2]), 11, 6);      // fragment 2 
      if (fr_sv[2] < 0) 
        bit32Pack(1, 17, 1);                 // neg
      else 
        bit32Pack(0, 17, 1);                 // pos   
      bit32Pack(Abs(fr_sv[3]), 18, 6);       // fragment 3
      if (fr_sv[3] < 0)
        bit32Pack(1, 24, 1);                 // neg
      else 
        bit32Pack(0, 24, 1);                 // pos      
      bit32Pack(Abs(fr_sv[4]), 25, 6);       // fragment 4 
      if (fr_sv[4] < 0)
        bit32Pack(1, 31, 1);                 // neg
      else 
        bit32Pack(0, 31, 1);                 // pos  
        
      uint8_t sv_num = sv_count % 4;

      SPort::PushToEmptyRow(msg_id, sv_num + 1);  

      #if defined Frs_Debug_All || defined Frs_Debug_Servo
        PrintFrPeriod(0);  
        Debug.print("Passthru out Servo_Raw 0x50F1: ");  
        Debug.print(" sv_chcnt="); Debug.print(sv_chcnt); 
        Debug.print(" sv_count="); Debug.print(sv_count); 
        Debug.print(" chunk="); Debug.print(chunk);
        Debug.print(" fr_sv1="); Debug.print(fr_sv[1]);
        Debug.print(" fr_sv2="); Debug.print(fr_sv[2]);
        Debug.print(" fr_sv3="); Debug.print(fr_sv[3]);   
        Debug.print(" fr_sv4="); Debug.print(fr_sv[4]); 
        Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
        SPort::PrintPayload(passthru);
        Debug.println();             
      #endif

      sv_count += 4; 
    }
    //===================================================================   
    
    void SPort::Push_VFR_Hud_50F2(uint16_t msg_id) {
      fr_payload = 0;
  
      fr_air_spd = ap_hud_air_spd * 10;      // from #74  m/s to dm/s
      fr_throt = ap_hud_throt;               // 0 - 100%
      fr_bar_alt = ap_hud_amsl * 10;      // m to dm

      #if defined Frs_Debug_All || defined Frs_Debug_Hud
        PrintFrPeriod(0);  
        Debug.print("Passthru out Hud 0x50F2: ");   
        Debug.print(" fr_air_spd="); Debug.print(fr_air_spd);
        Debug.print(" fr_throt=");   Debug.print(fr_throt);
        Debug.print(" fr_bar_alt="); Debug.print(fr_bar_alt);
        Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
        SPort::PrintPayload(passthru);
        Debug.println();             
      #endif
  
      fr_air_spd = prep_number(roundf(fr_air_spd), 2, 1);  
      bit32Pack(fr_air_spd, 0, 8);    

      bit32Pack(fr_throt, 8, 7);

      fr_bar_alt =  prep_number(roundf(fr_bar_alt), 3, 2);
      bit32Pack(fr_bar_alt, 15, 12);
      if (fr_bar_alt < 0)
        bit32Pack(1, 27, 1);  
      else
       bit32Pack(0, 27, 1); 
    
      SPort::PushToEmptyRow(msg_id, 1); 
        
    }
    //===================================================================  
    void SPort::Push_Wind_Estimate_50F3(uint16_t msg_id) {
      fr_payload = 0;
    }
    //===================================================================
               
    void SPort::Push_Rssi_F101(uint16_t msg_id) {          // msg_id 0xF101 RSSI tell LUA script in Taranis we are connected
      fr_payload = 0;
  
      if (rssiGood)
        fr_rssi = ap_rssi;            // always %
      else
        fr_rssi = 254;     // We may have a connection but don't yet know how strong. Prevents spurious "Telemetry lost" announcement
      #ifdef RSSI_Override   // dummy rssi override for debugging
        fr_rssi = 70;
      #endif

      if(fr_rssi <= 0){    // Patch from hasi123 modified 2020-07-28
        fr_rssi = RSSI_Override;
      }

      bit32Pack(fr_rssi ,0, 32);

      #if defined Frs_Debug_All || defined Frs_Debug_Rssi
        PrintFrPeriod(0);    
        Debug.print("Passthru out Rssi 0x5F101: ");   
        Debug.print(" fr_rssi="); Debug.print(fr_rssi);
        Debug.print(" fr_payload="); Debug.print(fr_payload);  Debug.print(" "); 
        SPort::PrintPayload(passthru);
        Debug.println();             
      #endif

      SPort::PushToEmptyRow(msg_id, 1); 
    }
 
    //===================================================================  
    
    void SPort::ReportSPortOnlineStatus() {
  
       if (spGood != spPrev) {  // report on change of status
         spPrev = spGood;
         if (spGood) {
          Debug.println("S.Port read good!");
          DisplayPrintln("S.Port read good!");         
         } else {
          Debug.println("S.Port read timeout!");
          DisplayPrintln("S.Port read timeout!");         
         }
       }
    }     
    //===================================================================  
    
    uint16_t SPort::MatchWaitingParamRequests(char * paramid) {
      for (int i = 0 ; i < mt20_rows ; i++ ) {  // try to find match on waiting param requests
    //    Debug.printf("MatchWaiting. i=%d  mt20_rows=%d  inuse=%d param_id=%s paramid=%s \n", i, mt20_rows, mt20[i].inuse, mt20[i].param_id,  ap22_param_id);

        bool paramsequal = (strcmp (mt20[i].param_id, paramid) == 0);
        if ( (mt20[i].inuse) && paramsequal ) {
          return i;             
         }
      }
      return 0xffff;  // this mean no match  
    }  
    //===================================================================  

    void SPort::PrintPayload(msg_class_t msg_class)  {
      uint8_t *bytes = 0;
      uint8_t sz;
      if (msg_class == passthru) {
        Debug.print(" passthru payload ");
        bytes = (uint8_t*)&fr_payload;         // cast to bytes
        sz = 4;   
      } else
      if (msg_class == mavlite) {
        Debug.print(" MavLite payload ");   
        bytes = (uint8_t*)&mt_Payload;           // cast to bytes
        sz = 6;
      } 
   
      for (int i = 0 ; i < sz ; i++) {
        PrintByte(bytes[i], 0);     
      }
      Debug.print("\t");
      for (int i = 0 ; i <sz ; i++) {
         if ((bytes[i] > 31) && (bytes[i] < 127)) Debug.write(bytes[i]);  
      }
      Debug.print("\t");
    } 
    //===================================================================  

    void SPort::PrintMavLiteUplink() {
  
        if (sp_buff[3] == 0) Debug.println();  // seq == 0       
        PrintByteNon(sp_buff[0]);  // 0x7E
        Debug.print(" ");
        PrintByteNon(sp_buff[1]);  // 0x0D
        Debug.print(" ");
        PrintByteNon(sp_buff[2]);  // 0x30 
        Debug.print("\t");
        PrintByteNon(sp_buff[3]);  // seq
        Debug.print(" ");       
        for (int i = 4 ; i <= 8 ; i++ ) {
          PrintByteNon(sp_buff[i]);
        }
        Debug.print(" | ");
        PrintByteNon(sp_buff[9]);  // ?

        Debug.print("\t");
        
        PrintByteNon(sp_buff[3]);  // seq

        if (sp_buff[3] == 0)  {    // if seq == 0
          Debug.print("  ");
          PrintByteNon(sp_buff[4]);   // length
          PrintByteNon(sp_buff[5]);   // msg_id 
          Debug.print(" [");
          for (int i = 6 ; i <= 8 ; i++ ) {      
            Debug.write(sp_buff[i]);   // print ascii
          }
        } else {                  // seq > 0
          Debug.print(" [");
          for (int i = 4 ; i <= 8 ; i++ ) {  
            Debug.write(sp_buff[i]);   // print ascii
          }       
        }
        Debug.print("] ");
        PrintByteNon(sp_buff[9]);  // ?
        Debug.println();
    }
    //===================================================================  
 
  
    
