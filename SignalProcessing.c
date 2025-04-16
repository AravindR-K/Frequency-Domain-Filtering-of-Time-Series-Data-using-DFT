#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 128 // Adjust based on your data points

const float samplingRate = 1000.0; // in Hz

// Cutoff frequencies for filtering
const float low_pass_cutoff = 50.0;  // Low-pass filter cutoff at 50 Hz
const float high_pass_cutoff = 200.0; // High-pass filter cutoff at 200 Hz

// Structure to hold time-domain data
typedef struct {
    float time[N];
    float readings[N];
} TimeDomainData;

// Structure to hold frequency-domain data
typedef struct {
    float X_real[N];
    float X_imag[N];
    float magnitude[N];
    float phase[N];
} FrequencyDomainData;

// Function to read data from file
int readData(const char *filename, TimeDomainData *timeData) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    for (int i = 0; i < N; i++) {
        if (fscanf(file, "%f %f", &timeData->time[i], &timeData->readings[i]) != 2) {
            break; // Stop reading if there's no more data
        }
    }

    fclose(file);
    return 0;
}

// Function to compute DFT
void computeDFT(const TimeDomainData *timeData, FrequencyDomainData *freqData) {
    for (int k = 0; k < N; k++) {
        freqData->X_real[k] = 0;
        freqData->X_imag[k] = 0;
        for (int n = 0; n < N; n++) {
            float angle = 2 * M_PI * k * n / N;
            freqData->X_real[k] += timeData->readings[n] * cos(angle);
            freqData->X_imag[k] -= timeData->readings[n] * sin(angle);
        }
    }
}

// Function to apply a low-pass filter on DFT results
void applyLowPassFilter(FrequencyDomainData *freqData) {
    float frequency;
    for (int k = 0; k < N; k++) {
        frequency = k * samplingRate / N;
        if (frequency > low_pass_cutoff) {
            freqData->X_real[k] = 0;
            freqData->X_imag[k] = 0;
        }
    }
}

// Function to apply a high-pass filter on DFT results
void applyHighPassFilter(FrequencyDomainData *freqData) {
    float frequency;
    for (int k = 0; k < N; k++) {
        frequency = k * samplingRate / N;
        if (frequency < high_pass_cutoff) {
            freqData->X_real[k] = 0;
            freqData->X_imag[k] = 0;
        }
    }
}

// Function to calculate magnitude and phase
void calculateMagnitudeAndPhase(FrequencyDomainData *freqData) {
    for (int k = 0; k < N; k++) {
        freqData->magnitude[k] = sqrt(freqData->X_real[k] * freqData->X_real[k] + freqData->X_imag[k] * freqData->X_imag[k]);
        freqData->phase[k] = atan2(freqData->X_imag[k], freqData->X_real[k]) * 180 / M_PI; // Phase in degrees
    }
}

// Function to write data for Gnuplot
void writeDataForGnuplot(const TimeDomainData *timeData, const FrequencyDomainData *freqData) {
    FILE *time_domain_file = fopen("time_domain.txt", "w");
    FILE *frequency_domain_file = fopen("frequency_domain.txt", "w");

    if (!time_domain_file || !frequency_domain_file) {
        perror("Error opening output file");
        exit(1);
    }

    // Write time-domain data
    for (int i = 0; i < N; i++) {
        fprintf(time_domain_file, "%f %f\n", timeData->time[i], timeData->readings[i]);
    }

    // Write frequency-domain data (only up to N/2 for positive frequencies)
    for (int k = 0; k < N / 2; k++) {
        float frequency = k * samplingRate / N;
        fprintf(frequency_domain_file, "%f %f\n", frequency, freqData->magnitude[k]);
    }

    fclose(time_domain_file);
    fclose(frequency_domain_file);
}

// Function to plot time-domain and frequency-domain data using Gnuplot
void plotWithGnuplot(int filterType) {
    // Plot time-domain graph
    FILE *gnuplotPipe1 = popen("gnuplot -persistent", "w");
    if (gnuplotPipe1) {
        fprintf(gnuplotPipe1, "set title 'Time Domain Signal'\n");
        fprintf(gnuplotPipe1, "set xlabel 'Time (s)'\n");
        fprintf(gnuplotPipe1, "set ylabel 'Amplitude'\n");
        fprintf(gnuplotPipe1, "plot 'time_domain.txt' with lines lw 2 title 'Signal'\n");
        fflush(gnuplotPipe1);
        pclose(gnuplotPipe1);
    } else {
        printf("Error: Could not open Gnuplot for time domain.\n");
    }

    // Plot frequency-domain graph with enhanced clarity
    FILE *gnuplotPipe2 = popen("gnuplot -persistent", "w");
    if (gnuplotPipe2) {
        fprintf(gnuplotPipe2, "set title '%s Filter - Frequency Domain Signal'\n",
                filterType == 1 ? "Low-Pass" : "High-Pass");
        fprintf(gnuplotPipe2, "set xlabel 'Frequency (Hz)'\n");
        fprintf(gnuplotPipe2, "set ylabel 'Magnitude'\n");

        // Set range to focus on the relevant frequencies (adjust based on sampling rate and N)
        fprintf(gnuplotPipe2, "set xrange [0:%f]\n", samplingRate / 2);
        fprintf(gnuplotPipe2, "set yrange [0:*]\n");

        // Plot cutoff frequency as a vertical line for clarity
        float cutoff = (filterType == 1) ? low_pass_cutoff : high_pass_cutoff;
        fprintf(gnuplotPipe2, "set arrow from %f,graph(0,0) to %f,graph(1,1) nohead lc rgb 'red' lw 2\n", cutoff, cutoff);
        fprintf(gnuplotPipe2, "set label 'Cutoff Frequency' at %f, graph 0.9 textcolor rgb 'red'\n", cutoff);

        // Plot frequency data
        fprintf(gnuplotPipe2, "plot 'frequency_domain.txt' with lines lw 2 title 'Magnitude'\n");
        fflush(gnuplotPipe2);
        pclose(gnuplotPipe2);
    } else {
        printf("Error: Could not open Gnuplot for frequency domain.\n");
    }
}

int main() {
    TimeDomainData timeData;
    FrequencyDomainData freqData;
    int choice;
    char continueChoice;

    // Read data from file
    if (readData("Readings.txt", &timeData) != 0) {
        return 1;
    }

    do {
        // Prompt user for filter choice
        printf("Select filter type:\n");
        printf("1: Low-Pass Filter\n");
        printf("2: High-Pass Filter\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice != 1 && choice != 2) {
            printf("Invalid choice. Please enter 1 or 2.\n");
            continue;
        }

        // Compute DFT
        computeDFT(&timeData, &freqData);

        // Apply chosen filter
        if (choice == 1) {
            applyLowPassFilter(&freqData);
        } else if (choice == 2) {
            applyHighPassFilter(&freqData);
        }

        // Calculate magnitude and phase after filtering
        calculateMagnitudeAndPhase(&freqData);

        // Write data to files for Gnuplot
        writeDataForGnuplot(&timeData, &freqData);

        // Plot the time and frequency domain results
        plotWithGnuplot(choice);

        // Ask the user if they want to continue
        printf("Do you want to apply another filter? (y/n): ");
        scanf(" %c", &continueChoice);

    } while (continueChoice == 'y' || continueChoice == 'Y');

    printf("Exiting the program. Goodbye!\n");

    return 0;
}
