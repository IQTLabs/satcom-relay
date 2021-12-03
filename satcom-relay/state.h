struct State {
  byte battery;
  time_t lastGPSUpdate;
  float latitude;
  float longitude;
  uint32_t gpsSleepTimer;
  uint32_t heartbeatSleepTime;
};

void printState(State *s) {
  Serial.print("Battery: ");
  Serial.println(s->battery);
}
