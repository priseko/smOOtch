void dispFade(uint8_t fromValue, uint8_t toValue, uint8_t holdTime)
{
  uint8_t idx = fromValue;
  int8_t sign = fromValue < toValue ? 1 : -1;
  while (idx != toValue)
  {
    disp.setBrightness(0xf0 | idx);
    delay(holdTime);
    idx+=sign;
  }
}

uint8_t dispWait(uint8_t delayMs)
{
  readTouchInputs();
}

void dispTime(uint8_t brightness)
{
  int8_t idx;
  
  //disp hours
  disp.clear();
  disp.write(0, 0, fontBigNum[GET_TENTH(hh)]);
  disp.write(6, 0, fontBigNum[GET_ONES(hh)], 0xffff);    
  disp.setBrightness(0xf0);
  disp.sync();
  dispFade(0, brightness, 20); 
  delay(2000);

  //scroll
  for (idx=0; idx>-15; idx--)
  {
    disp.write(idx, 0, fontBigNum[GET_TENTH(hh)]);
    disp.write(idx+6, 0, fontBigNum[GET_ONES(hh)], 0xffff);    
    disp.write(idx+12, 0, fontColon, 0xffff);
    disp.write(idx+14, 0, fontBigNum[GET_TENTH(mm)], 0xffff);
    disp.write(idx+20, 0, fontBigNum[GET_ONES(mm)], 0xffff);
    disp.sync();
    delay(110);
  }  
  delay(2000);
  
  //fade to seconds
  previousSecond = ss;
  while (previousSecond == ss);
  dispFade(brightness, 0, 20);
  disp.clear();
  disp.sync();
  disp.write(0, 0, fontBigNum[GET_TENTH(ss)]);
  disp.write(6, 0, fontBigNum[GET_ONES(ss)], 0xffff);   
  disp.sync();
  dispFade(0, brightness, 20);
  
  // disp seconds for 5s
  idx = 0;
  while (idx<6)
  {
    if (ss != previousSecond) {
      disp.write(0, 0, fontBigNum[GET_TENTH(ss)]);
      disp.write(6, 0, fontBigNum[GET_ONES(ss)], 0xffff);   
      disp.sync();
      previousSecond = ss;
      idx++;
    }
  }
  dispFade(brightness, 0, 20);
  disp.clear();
  disp.sync();
}


  
