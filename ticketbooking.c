

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep(1000 * (x))
#else
    #include <unistd.h>
#endif


#define MAX_NAME_LEN 100
#define MAX_PHONE_LEN 20

typedef enum {SEAT_AVAILABLE = 0, SEAT_RESERVED = 1} SeatStatus;

/* Seat structure */
typedef struct {
    char label[4];        /* e.g., "A1" */
    double price;
    SeatStatus status;
} Seat;

/* Booking node (linked list) */
typedef struct Booking {
    int booking_id;
    char customer_name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    int *seat_indices;  
    int seat_count;
    double amount_paid;
    time_t booking_time;
    struct Booking *next;
} Booking;

typedef struct {
    char event_name[128];
    int rows;     
    int cols;    
    int seat_count;
    Seat *seats; 
    Booking *bookings_head;
    int next_booking_id; 
} Event;


static void flush_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

Event *create_event(const char *name, int rows, int cols) {
    Event *ev = malloc(sizeof(Event));
    if (!ev) { perror("malloc"); exit(EXIT_FAILURE); }
    strncpy(ev->event_name, name, sizeof(ev->event_name)-1);
    ev->event_name[sizeof(ev->event_name)-1] = '\0';
    ev->rows = rows;
    ev->cols = cols;
    ev->seat_count = rows * cols;
    ev->seats = malloc(sizeof(Seat) * ev->seat_count);
    if (!ev->seats) { perror("malloc"); exit(EXIT_FAILURE); }
    ev->bookings_head = NULL;
    ev->next_booking_id = 1001; 

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = r * cols + c;
            char label[4];
            label[0] = 'A' + r;
            /* labels like A1, A2... */
            snprintf(label+1, sizeof(label)-1, "%d", c+1);
            label[0] = 'A' + r;
            label[1] = '\0';
            
            snprintf(ev->seats[idx].label, sizeof(ev->seats[idx].label), "%c%d", 'A' + r, c+1);
           
            double base = 100.0; 
            double multiplier = 1.0 + (double)(rows - 1 - r) * 0.15; /* earlier rows cost more */
            ev->seats[idx].price = base * multiplier;
            ev->seats[idx].status = SEAT_AVAILABLE;
        }
    }
    return ev;
}


void destroy_event(Event *ev) {
    if (!ev) return;

    Booking *cur = ev->bookings_head;
    while (cur) {
        Booking *n = cur->next;
        free(cur->seat_indices);
        free(cur);
        cur = n;
    }
    free(ev->seats);
    free(ev);
}


void print_seat_map(Event *ev) {
    printf("\nEvent: %s — Seat Map (A = available, R = reserved)\n", ev->event_name);
    for (int r = 0; r < ev->rows; ++r) {
        printf("%c: ", 'A' + r);
        for (int c = 0; c < ev->cols; ++c) {
            int idx = r * ev->cols + c;
            char mark = (ev->seats[idx].status == SEAT_AVAILABLE) ? 'A' : 'R';
            printf("%2s[%c] ", ev->seats[idx].label, mark);
        }
        printf("\n");
    }
    printf("\n");
}


int find_seat_index(Event *ev, const char *label) {
    if (!label || strlen(label) < 2) return -1;

    char letter = toupper(label[0]);
    int row = letter - 'A';
    if (row < 0 || row >= ev->rows) return -1;
    char *p = (char*)(label + 1);
    int num = atoi(p);
    if (num <= 0 || num > ev->cols) return -1;
    int idx = row * ev->cols + (num - 1);
    return idx;
}


void print_seat_details(Event *ev, int idx) {
    if (idx < 0 || idx >= ev->seat_count) return;
    Seat *s = &ev->seats[idx];
    printf("Seat %s — Price: %.2f — %s\n", s->label, s->price,
           s->status == SEAT_AVAILABLE ? "Available" : "Reserved");
}


int reserve_seats(Event *ev, int *indices, int count) {
   
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        if (idx < 0 || idx >= ev->seat_count) return -1;
        if (ev->seats[idx].status != SEAT_AVAILABLE) return 0; 
    /* Reserve */
    for (int i = 0; i < count; ++i) {
        ev->seats[indices[i]].status = SEAT_RESERVED;
    }
    return 1;
}

/* Unreserve seats (used when payment fails or cancellation) */
void unreserve_seats(Event *ev, int *indices, int count) {
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        if (idx >= 0 && idx < ev->seat_count) {
            ev->seats[idx].status = SEAT_AVAILABLE;
        }
    }
}


Booking *create_booking(Event *ev, const char *name, const char *phone, int *seat_indices, int seat_count, double amount_paid) {
    Booking *b = malloc(sizeof(Booking));
    if (!b) { perror("malloc"); exit(EXIT_FAILURE); }
    b->booking_id = ev->next_booking_id++;
    strncpy(b->customer_name, name, MAX_NAME_LEN-1);
    b->customer_name[MAX_NAME_LEN-1] = '\0';
    strncpy(b->phone, phone, MAX_PHONE_LEN-1);
    b->phone[MAX_PHONE_LEN-1] = '\0';
    b->seat_indices = malloc(sizeof(int) * seat_count);
    if (!b->seat_indices) { perror("malloc"); exit(EXIT_FAILURE); }
    for (int i = 0; i < seat_count; ++i) b->seat_indices[i] = seat_indices[i];
    b->seat_count = seat_count;
    b->amount_paid = amount_paid;
    b->booking_time = time(NULL);
    b->next = ev->bookings_head;
    ev->bookings_head = b;
    return b;
}


int process_payment_simulation(double amount) {
    printf("\n--- Payment Processing ---\n");
    printf("Amount to pay: %.2f\n", amount);
    printf("Select payment method:\n");
    printf("1) Card\n2) UPI\n3) Netbanking\nChoose (1-3): ");
    int choice = 0;
    if (scanf("%d", &choice) != 1) { flush_input(); return 0; }
    flush_input();

    if (choice == 1) {
        char card[32];
        char cvv[8];
        char name[MAX_NAME_LEN];
        printf("Enter card number (simulated): ");
        fgets(card, sizeof(card), stdin);
        card[strcspn(card, "\n")] = '\0';
        printf("Enter name on card: ");
        fgets(name, sizeof(name), stdin); name[strcspn(name, "\n")] = '\0';
        printf("Enter CVV: ");
        fgets(cvv, sizeof(cvv), stdin); cvv[strcspn(cvv, "\n")] = '\0';
    } else if (choice == 2) {
        char upi[64];
        printf("Enter UPI ID (simulated): ");
        fgets(upi, sizeof(upi), stdin);
        upi[strcspn(upi, "\n")] = '\0';
    } else {
        char bank[64];
        printf("Enter bank name (simulated): ");
        fgets(bank, sizeof(bank), stdin);
        bank[strcspn(bank, "\n")] = '\0';
    }

    printf("Processing");
    for (int i = 0; i < 3; ++i) { printf("."); fflush(stdout); sleep(1); }
    printf("\n");

    
    int r = rand() % 100;
    if (r < 90) {
        printf("Payment successful ✅\n");
        return 1;
    } else {
        printf("Payment failed ❌ (simulated)\n");
        return 0;
    }
}


void handle_new_booking(Event *ev) {
    char name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    printf("Enter customer name: ");
    fgets(name, sizeof(name), stdin); name[strcspn(name, "\n")] = '\0';
    if (strlen(name) == 0) { printf("Name cannot be empty.\n"); return; }
    printf("Enter phone number: ");
    fgets(phone, sizeof(phone), stdin); phone[strcspn(phone, "\n")] = '\0';

    print_seat_map(ev);
    printf("Enter seats to book separated by spaces (e.g., A1 A2 A3). Max 20 seats: \n");
    char line[512];
    fgets(line, sizeof(line), stdin);
    line[strcspn(line, "\n")] = '\0';
    if (strlen(line) == 0) { printf("No seats entered.\n"); return; }

    int indices[256];
    int count = 0;
    char *tok = strtok(line, " \t,");
    while (tok && count < 200) {
        int idx = find_seat_index(ev, tok);
        if (idx == -1) {
            printf("Invalid seat label: %s — aborting selection.\n", tok);
            return;
        }
        indices[count++] = idx;
        tok = strtok(NULL, " \t,");
    }
    if (count == 0) { printf("No valid seats entered.\n"); return; }


    for (int i = 0; i < count; ++i) {
        for (int j = i+1; j < count; ++j) {
            if (indices[i] == indices[j]) {
                printf("Duplicate seat %s in selection — aborting.\n", ev->seats[indices[i]].label);
                return;
            }
        }
    }


    double total = 0.0;
    printf("\nSelected seats:\n");
    for (int i = 0; i < count; ++i) {
        print_seat_details(ev, indices[i]);
        total += ev->seats[indices[i]].price;
    }
    printf("\nTotal amount: %.2f\n", total);

    
    int reserve_ok = reserve_seats(ev, indices, count);
    if (reserve_ok == 0) {
        printf("One or more seats have just been reserved by someone else. Please try again.\n");
        return;
    } else if (reserve_ok < 0) {
        printf("Invalid seat selection.\n");
        return;
    }

    int payment_success = process_payment_simulation(total);
    if (!payment_success) {
    
        unreserve_seats(ev, indices, count);
        printf("Booking aborted due to payment failure. Seats released.\n");
        return;
    }

    
    create_booking(ev, name, phone, indices, count, total);
    printf("Booking confirmed! Booking ID: %d\n", ev->next_booking_id - 1);
    printf("A booking confirmation has been created for %s. Thank you!\n", name);
}


void list_bookings(Event *ev) {
    if (!ev->bookings_head) {
        printf("No bookings yet.\n");
        return;
    }
    printf("\n--- Bookings ---\n");
    Booking *cur = ev->bookings_head;
    while (cur) {
        char tbuf[64];
        struct tm *tm = localtime(&cur->booking_time);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("Booking ID: %d | Name: %s | Phone: %s | Seats: ", cur->booking_id, cur->customer_name, cur->phone);
        for (int i = 0; i < cur->seat_count; ++i) {
            printf("%s%s", ev->seats[cur->seat_indices[i]].label, (i+1==cur->seat_count)?"":" ");
        }
        printf(" | Amount: %.2f | Time: %s\n", cur->amount_paid, tbuf);
        cur = cur->next;
    }
    printf("\n");
}


void cancel_booking(Event *ev) {
    printf("Enter Booking ID to cancel: ");
    int id;
    if (scanf("%d", &id) != 1) { flush_input(); printf("Invalid input.\n"); return; }
    flush_input();

    Booking *prev = NULL;
    Booking *cur = ev->bookings_head;
    while (cur && cur->booking_id != id) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Booking ID %d not found.\n", id);
        return;
    }

   
    printf("Processing refund of %.2f\n", cur->amount_paid);
    printf("Refund processing");
    for (int i = 0; i < 3; ++i) { printf("."); fflush(stdout); sleep(1); }
    printf("\n");
    int r = rand() % 100;
    if (r >= 95) {
        printf("Refund failed due to simulated gateway error. Try again later.\n");
        return;
    }

/
    unreserve_seats(ev, cur->seat_indices, cur->seat_count);

   
    if (prev) prev->next = cur->next;
    else ev->bookings_head = cur->next;

    free(cur->seat_indices);
    free(cur);

    printf("Booking cancelled and seats released. Refund successful.\n");
}


void availability_summary(Event *ev) {
    int available = 0;
    int reserved = 0;
    for (int i = 0; i < ev->seat_count; ++i) {
        if (ev->seats[i].status == SEAT_AVAILABLE) available++;
        else reserved++;
    }
    printf("Total seats: %d | Available: %d | Reserved: %d\n", ev->seat_count, available, reserved);
}

void menu_loop(Event *ev) {
    while (1) {
        printf("\n=== Ticket Booking System — Event: %s ===\n", ev->event_name);
        printf("1) Show seat map\n2) Show seat details\n3) New booking\n4) List bookings\n5) Cancel booking\n6) Availability summary\n7) Exit\nChoose: ");
        int choice;
        if (scanf("%d", &choice) != 1) { flush_input(); printf("Invalid choice.\n"); continue; }
        flush_input();
        switch (choice) {
            case 1:
                print_seat_map(ev);
                break;
            case 2: {
                printf("Enter seat label (e.g., A3): ");
                char lab[8];
                fgets(lab, sizeof(lab), stdin); lab[strcspn(lab, "\n")] = '\0';
                int idx = find_seat_index(ev, lab);
                if (idx == -1) { printf("Invalid seat label.\n"); }
                else print_seat_details(ev, idx);
                break;
            }
            case 3:
                handle_new_booking(ev);
                break;
            case 4:
                list_bookings(ev);
                break;
            case 5:
                cancel_booking(ev);
                break;
            case 6:
                availability_summary(ev);
                break;
            case 7:
                printf("Exiting. Goodbye!\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}


int main(void) {
    srand((unsigned int)time(NULL));

    
    Event *ev = create_event("College Cultural Night YAGVIK", 4, 10); 

    printf("Welcome to the Ticket Booking System (single event)\n");
    printf("Event: %s | Seats: %d\n", ev->event_name, ev->seat_count);
    menu_loop(ev);

    destroy_event(ev);
    return 0;
}


