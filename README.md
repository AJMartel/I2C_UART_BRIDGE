# I2C_UART_BRIDGE
this sketch allows your arduino to act as A UART to I2C or I2C TO UART BRIDGE

/*
 * This is code was made with the intention to convert I2C to UART Or UART to I2C
 * By sending in the first String of the COMMAND[] String, settings can be changed
 * using the follring syntax:   COMMAND[0]COMMAND[x]val,COMMAND[n]
 * It is adviced to clear the EEPROM once the code has been uploade
 * this is done by sending COMMAND[0]COMMAND[7]==> !#RST
 * COMMANDS[] = {"!#", "BAUD", "ADDR", "I2CC", "BUFL", "MSTR", "SLVD", "RST", "TIMO", "SLND"}
 *                setting  // BAUD(val) // ADDR(1-128)//I2CC(100000, 400000)//BUFL(1 - MAXI2CBUFFERLENGTH)//MSTR//SLVD//RST//TIMO(val)//SLND(0-1)
 *                         //uart BAUD // I2C address  // I2C clock speed /// I2C BUFFER SIZE  // I2C MASTER MODE// I2C SLAVE MODE // RESET SETTINGS // TIMOUT // SILEND (no echo)
 *  
 */
