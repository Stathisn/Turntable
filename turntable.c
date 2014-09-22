/* turntable.c describes functions which are executed by the turntable
*  SAMPLING OF THE IO PINS WILL BE EXECUTED USING THE BCM2835_GPIO LIBRARY
*  EVENT DETECT STATUS WILL BE USED FOR ASYNCHRONOUS READING OF THE PINS
*  LOOK UP FUNCTION:
*  -----uint8_t bcm2835_gpio_eds(uint8_t pin)-------
*/

#include "turntable.h"
#include <jansson.h>
#include <bcm2835.h>

// GPIO pins for turntable control
#define ENCODER RPI_GPIO_P1_11
#define SWITCH  RPI_GPIO_P1_13
#define MOTOR   RPI_GPIO_P1_15


TurnInstruction_t newTurnInstruction(TurntableCommands_t command, int rotation, int direction)
{
  TurnInstruction_t t;
  t.command = command;
  t.rotation = rotation;
  t.direction = direction;
  return t;
}

int initTurntable(Turntable_t *t)
{
  printf("initTurntable start");
  if(!bcm2835_init())
    return 1;
  // Set the pin modes
  bcm2835_gpio_fsel(MOTOR, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(ENCODER, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_fsel(SWITCH, BCM2835_GPIO_FSEL_INPT);
  // Initialise members
  t->maxEncoder = 0;
  t->currentEncoder = 0;
  t->encoder = 0;
  t->limitSW = 0;
  t->ttdriver = 0;
  printf("initTurntable end");
  return 0;
}


// Interprets an instruction structure and executes appropriate action on the
// given turntable object
int parseTurnInstruction(TurnInstruction_t* instruction, Turntable_t* turntable)
{
  switch(instruction->command)
  {
    case reset:
      return reset_tt(turntable);
    case calibrate:
      return calibrate_tt(turntable);
    case quarterTurn:
      return quarterTurn_tt(turntable, instruction->direction, instruction->rotation);
    case fineTurn:
      return fineTurn_tt(turntable, instruction->direction, instruction->rotation);
    default:
      return 1; // Not a recongised command
  }
}

int reset_tt(Turntable_t* turntable)
{
  // This function returns the turntable to its starting state
  // Start the motor
  bcm2835_gpio_set(MOTOR);
  // Wait for reset
  while(bcm2835_gpio_lev(SWITCH))
  {
    // Waits for falling edge
  }
  while(!bcm2835_gpio_lev(SWITCH))
  {
    // waits for rising edge
  }
  // Stop the motor
  bcm2835_gpio_clr(MOTOR);
  // Reset the turntable valus
  turntable->maxEncoder = 0;
  turntable->currentEncoder = 0;
  return 0;
}

int calibrate_tt(Turntable_t* turntable)
{
  // This function must be run immediately after reset to calibrate the encoder readings
  // Clear the interrupt flags
  bcm2835_gpio_set_eds(SWITCH);
  bcm2835_gpio_set_eds(ENCODER);
  while(!bcm2835_gpio_eds(SWITCH))
  {
    while(!bcm2835_gpio_eds(ENCODER))
    {
      // Do nothing
    }
    turntable->maxEncoder++;
  }
  return 0;
}


int quarterTurn_tt(Turntable_t* turntable, int direction, int quarters)
{
  return 0;
}


int fineTurn_tt(Turntable_t* turntable, int direction, int ticks)
{
  return 0;
}


/* JSON object for TurnInstruction looks like the following:
*  { TurnInstruction:{
*    "command":int,
*    "rotation":int,
*    "direction":int
*    }
*  }
*/

// accepts the JSON serialised string "text" and populates TurnInstruction with its data
int jsonToTurnInstruction(TurnInstruction_t* instruction, char* text)
{
  // Create JSON structures
  json_t* root;
  json_error_t error;

  // load text into JSON structure
  root = json_loads(text, 0, &error);

  // Checks for correctness
  if(!root)
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }

  // Gets and checks the TurnInstruction object
  json_t* turnInstruction;
  turnInstruction = json_object_get(root, "TurnInstruction");
  if(!turnInstruction)
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }
  if(!json_is_object(turnInstruction))
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }

  // Get and text the contents of the TurnInstruction object
  json_t *command, *rotation, *direction;
  command = json_object_get(turnInstruction, "command");
  if(!json_is_integer(command))
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }
  rotation = json_object_get(turnInstruction, "rotation");
  if(!json_is_integer(rotation))
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }
  direction = json_object_get(turnInstruction, "direction");
  if(!json_is_integer(direction))
  {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }

  // Populates TurnInstruction structure with the captured parameters
  instruction->command = json_integer_value(command);
  instruction->rotation = json_integer_value(rotation);
  instruction->direction = json_integer_value(direction);

  return 0;
}
