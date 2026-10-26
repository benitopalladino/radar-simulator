#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_TARGETS 10
#define NUM_NOISE   30
#define MAX_CONTACTS (NUM_TARGETS + NUM_NOISE)
#define THRESHOLD   8.0
#define NOISE_MAX   6.0
#define K_RADAR     1e8   /* potenza eco = K / R^4 (R in km) */

typedef struct {
    int id;
    double range_km;
    double azimuth_deg;
    double speed_ms;
    double amplitude;
    int is_real;
} Contact;

static double rand_between(double min, double max) {
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

int main(void) {
    Contact contacts[MAX_CONTACTS];
    srand((unsigned)time(NULL));

    /* Bersagli reali: l'eco si indebolisce con la distanza (1/R^4) */
    for (int i = 0; i < NUM_TARGETS; i++) {
        double r = rand_between(1.0, 100.0);
        double r2 = r * r;
        contacts[i] = (Contact){
            .id = i + 1,
            .range_km = r,
            .azimuth_deg = rand_between(0.0, 359.0),
            .speed_ms = rand_between(0.0, 300.0),
            .amplitude = K_RADAR / (r2 * r2) + rand_between(0.0, NOISE_MAX),
            .is_real = 1
        };
    }

    /* Contatti di solo rumore */
    for (int i = NUM_TARGETS; i < MAX_CONTACTS; i++) {
        contacts[i] = (Contact){
            .id = i + 1,
            .range_km = rand_between(1.0, 100.0),
            .azimuth_deg = rand_between(0.0, 359.0),
            .speed_ms = 0.0,
            .amplitude = rand_between(0.0, NOISE_MAX),
            .is_real = 0
        };
    }

    /* Rilevamento a soglia */
    int detected_real = 0, false_alarms = 0;
    printf("Soglia: %.1f\n\n", THRESHOLD);
    for (int i = 0; i < MAX_CONTACTS; i++) {
        if (contacts[i].amplitude > THRESHOLD) {
            if (contacts[i].is_real) detected_real++;
            else false_alarms++;
            printf("Rilevato #%d: %.1f km, %.1f deg, ampiezza %.1f [%s]\n",
                   contacts[i].id, contacts[i].range_km,
                   contacts[i].azimuth_deg, contacts[i].amplitude,
                   contacts[i].is_real ? "BERSAGLIO" : "FALSO ALLARME");
        }
    }

    printf("\nBersagli rilevati: %d/%d\n", detected_real, NUM_TARGETS);
    printf("Bersagli mancati:  %d\n", NUM_TARGETS - detected_real);
    printf("Falsi allarmi:     %d\n", false_alarms);
    return 0;
}