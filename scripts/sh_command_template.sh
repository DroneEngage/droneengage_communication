#!/bin/bash

# This script displays the command name ($0) and all the parameters ($@) passed to it, with added colors.

# Define ANSI color codes
NC='\e[0m'          # No Color/Reset
BLUE_BG='\e[44m'    # Blue Background
WHITE_FG='\e[97m'   # White Foreground
BOLD='\e[1m'        # Bold text
CYAN='\e[96m'       # Light Cyan for titles
GREEN='\e[92m'      # Light Green for command name
YELLOW='\e[93m'     # Light Yellow for total count
MAGENTA='\e[95m'    # Light Magenta for parameter values
GRAY='\e[90m'       # Dark Gray for 'no parameters' message

echo -e "${BOLD}==================================================="
echo -e "${BLUE_BG}${WHITE_FG}${BOLD}           Command Line Analysis Tool              ${NC}"
echo -e "${BOLD}===================================================${NC}"

# $0 holds the name of the script or command as it was called
echo -e "${CYAN}${BOLD}1. Command/Script Name:${NC}"
echo -e "   ${GREEN}${BOLD}$0${NC}"

echo ""

# $@ holds all positional parameters starting from $1
echo -e "${CYAN}${BOLD}2. Total Number of Parameters:${NC}"
echo -e "   ${YELLOW}${BOLD}$#${NC}"

echo ""

echo -e "${CYAN}${BOLD}3. Listing All Parameters:${NC}"

# Check if there are any arguments
if [ "$#" -eq 0 ]; then
    echo -e "   ${GRAY}(No parameters provided)${NC}"
else
    # Loop through all arguments to display them clearly with their index
    
    # Start argument indexing from 1 (since $0 is the command name)
    arg_index=1
    
    for arg in "$@"; do
        # printf is used for controlled output formatting, coloring the index white/bold and the value magenta
        printf "   ${WHITE_FG}${BOLD}Arg %-2d:${NC} ${MAGENTA}%s${NC}\n" $arg_index "$arg"
        arg_index=$((arg_index + 1))
    done
fi

echo -e "${BOLD}===================================================${NC}"
