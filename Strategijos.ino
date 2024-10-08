bool push_state = false;
unsigned long judejimo_buves = 0;
int laukimo_laikas = 3000;
int judejimo_laikas = 100;

int intervalas_3kg = laukimo_laikas;

void Mega_sumo_taktika() {
  foward();
  delay(judejimo_laikas);
  Stop();
  delay(1);
  judejimo_buves = millis();

  bool reikia_atgal = false;
  int judejimo_skaicius = 1;

  while (true) {

    readSensors();

    if (RUN_STATE != 0) {
      break;
    }
    if (millis() - judejimo_buves >= intervalas_3kg) {
      judejimo_buves = millis();
      if (push_state == false) {
        push_state = true;
        intervalas_3kg = judejimo_laikas;
        if (judejimo_skaicius == 2) {
          back();
          judejimo_skaicius = 0;
        }
        else {
         foward();
          judejimo_skaicius++;
        }
      }
      else {
        push_state = false;
        Stop();
        intervalas_3kg = laukimo_laikas;
      }
    }
  }

}
