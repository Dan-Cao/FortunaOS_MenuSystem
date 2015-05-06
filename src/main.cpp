/* COMP2215 Task 7
 * FortunaOS with integrated menu system
 * */

extern "C"
{
#include "os.h"
#include "audio.h"
}

#include "menubackend.h"

int blink(int);

int update_dial(int);

int collect_delta(int);

int check_switches(int);

void tail(uint8_t lines);   /* Show last lines of a file */

void menuSetup();
void menuUseEvent(MenuUseEvent used);
void menuChangeEvent(MenuChangeEvent changed);

/* Lines have to be shorter than this. Note: buffer is on stack. */
#define LINE_BUFF_LEN 55


FIL File;
/* FAT File */

int position = 0;


// Menu generation
MenuBackend menu = MenuBackend(menuUseEvent, menuChangeEvent);
MenuItem fatoperations = MenuItem(menu, "FAT Operations", 1);
    MenuItem readfile = MenuItem(menu, "Read myfile.txt", 2);
    MenuItem writepos = MenuItem(menu, "Write encoder position", 2);
MenuItem audio = MenuItem(menu, "Play audio", 1);
    MenuItem sound1 = MenuItem(menu, "Play sound 1", 2);
    MenuItem sound2 = MenuItem(menu, "Play sound 2", 2);
    MenuItem sound3 = MenuItem(menu, "Play sound 3", 2);
    MenuItem sound4 = MenuItem(menu, "Play sound 4", 2);


int main(void) {
    os_init();

    os_add_task(blink, 30, 1);
    os_add_task(collect_delta, 500, 1);
    os_add_task(check_switches, 100, 1);

    menuSetup();

    sei();
    for (; ;) { }

}


void menuSetup(){

    // FAT operations
    menu.getRoot().add(fatoperations);
    fatoperations.addLeft(fatoperations);
    fatoperations.addAfter(audio);

    fatoperations.addRight(readfile);
    readfile.addLeft(fatoperations);
    writepos.addLeft(fatoperations);

    readfile.addAfter(writepos);
    readfile.addBefore(writepos);
    writepos.addAfter(readfile);
    writepos.addBefore(readfile);

    // Audio operations
    audio.addLeft(audio);
    audio.addBefore(fatoperations);

    audio.addRight(sound1);
    sound1.addAfter(sound2);
    sound2.addAfter(sound3);
    sound3.addAfter(sound4);
    sound4.addAfter(sound1);
    sound1.addBefore(sound4);
    sound2.addBefore(sound1);
    sound3.addBefore(sound2);
    sound4.addBefore(sound3);

    sound1.addLeft(audio);
    sound2.addLeft(audio);
    sound3.addLeft(audio);
    sound4.addLeft(audio);

    display_string("Hello! Use the directional buttons and the centre button to navigate the menu.\n");
}

int collect_delta(int state) {
    position += os_enc_delta();
    return state;
}


int check_switches(int state) {

    if (get_switch_press(_BV(SWN))) {
        menu.moveUp();
    }

    if (get_switch_press(_BV(SWE))) {
        menu.moveRight();
    }

    if (get_switch_press(_BV(SWS))) {
        menu.moveDown();
    }

    if (get_switch_press(_BV(SWW))) {
        menu.moveLeft();
    }

    if (get_switch_long(_BV(SWC))) {
        menu.use();
    }

    if (get_switch_short(_BV(SWC))) {
//        display_string("[S] Centre\n");
    }

    if (get_switch_rpt(_BV(SWN))) {
//        display_string("[R] North\n");
    }

    if (get_switch_rpt(_BV(SWE))) {
//        display_string("[R] East\n");
    }

    if (get_switch_rpt(_BV(SWS))) {
//        display_string("[R] South\n");
    }

    if (get_switch_rpt(_BV(SWW))) {
//        display_string("[R] West\n");
    }

    if (get_switch_rpt(SWN)) {
//        display_string("[R] North\n");
    }


    if (get_switch_long(_BV(OS_CD))) {
        display_color(FOREST_GREEN, BLACK);
        display_string("Detected SD card.\n");
        display_color(WHITE, BLACK);
    }

    return state;
}


int blink(int state) {
    static int light = 0;
    uint8_t level;

    if (light < -120) {
        state = 1;
    } else if (light > 254) {
        state = -20;
    }


    /* Compensate somewhat for nonlinear LED
       output and eye sensitivity:
    */
    if (state > 0) {
        if (light > 40) {
            state = 2;
        }
        if (light > 100) {
            state = 5;
        }
    } else {
        if (light < 180) {
            state = -10;
        }
        if (light < 30) {
            state = -5;
        }
    }
    light += state;

    if (light < 0) {
        level = 0;
    } else if (light > 255) {
        level = 255;
    } else {
        level = light;
    }

    os_led_brightness(level);
    return state;
}


/* A more general solution would check for the length of the file
   and if the file is long would work backwards from the end
   of the file in bufferable chunks for long files until it
   has found the required number of newline characters.
   For short files it would do the same as is done here, i.e.,
   read into a ring buffer until the end of the file has been
   encountered:
*/
void tail(uint8_t n) {
    char line[n][LINE_BUFF_LEN];
    int8_t i, j;


    f_mount(&FatFs, "", 0);
    if (f_open(&File, "myfile.txt", FA_READ) == FR_OK) {
        for (i = 0; i < n; i++) {
            line[n][0] = '\0';
        }

        i = 0;
        while (f_gets(line[i], LINE_BUFF_LEN, &File)) {
            i++;
            i %= n;
        }
        clear_screen();
        for (j = 0; j < n; j++) {
            display_string(line[i++]);
            i %= n;
        }
        f_close(&File);
    } else {
        display_string("Can't read file! \n");
    }

}

/*
  Called when menu is used
*/
void menuUseEvent(MenuUseEvent used) {
    //Logic to check what event to run

    /*if (used.item.isEqual(setDelay)) //comparison agains a known item
    {
        Serial.println("menuUseEvent found Delay (D)");
    }*/

    if (used.item.isEqual(fatoperations) || used.item.isEqual(audio)) {
        menu.moveRight();
    } else if (used.item.isEqual(readfile)) {
        tail(25);
    } else if (used.item.isEqual(writepos)) {
        f_mount(&FatFs, "", 0);
        if (f_open(&File, "myfile.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {
            f_lseek(&File, f_size(&File));
            f_printf(&File, "Encoder position is: %d \r\n", position);
            f_close(&File);
            display_color(BLACK, SKY_BLUE);
            display_string("Wrote position\n");
            display_color(WHITE, BLACK);
        } else {
            display_string("Can't write file! \n");
        }
    } else if (used.item.isEqual(sound1)) {
        display_color(GOLD, BLACK);
        display_string("Playing sound 1!\n");
        display_color(WHITE, BLACK);
        f_mount(&FatFs, "", 0);
        f_open(&File, "ufo.wav", FA_READ);
        audio_load(&File);
        f_close(&File);
    } else if (used.item.isEqual(sound2)) {
        display_color(BLUE_VIOLET, BLACK);
        display_string("Playing sound 2!\n");
        display_color(WHITE, BLACK);
        f_mount(&FatFs, "", 0);
        f_open(&File, "drum.wav", FA_READ);
        audio_load(&File);
        f_close(&File);
    } else if (used.item.isEqual(sound3)) {
        display_color(CRIMSON, BLACK);
        display_string("Playing sound 3!\n");
        display_color(WHITE, BLACK);
        f_mount(&FatFs, "", 0);
        f_open(&File, "glass.wav", FA_READ);
        audio_load(&File);
        f_close(&File);
    } else if (used.item.isEqual(sound4)) {
        display_color(CYAN, BLACK);
        display_string("Playing sound 4!\n");
        display_color(WHITE, BLACK);
        f_mount(&FatFs, "", 0);
        f_open(&File, "ocean.wav", FA_READ);
        audio_load(&File);
        f_close(&File);
    }
}

/*
 * Called when menu is changed. Updates screen with new item.
 * */
void menuChangeEvent(MenuChangeEvent changed) {
    clear_screen();
    display_string("____________________________\n-> ");
    display_string((char *) changed.to.getName());
    display_string("\n____________________________\nUse the directional keys to navigate the menu!\n\n");
}