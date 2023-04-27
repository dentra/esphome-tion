HISTCONTROL=ignoredups:erasedups
export HISTFILE=$WORKSPACE_DIR/.esphome/.bash_history
shopt -s histappend
# PROMPT_COMMAND="history -n; history -w; history -c; history -r; $PROMPT_COMMAND"
PROMPT_COMMAND="history -a; $PROMPT_COMMAND"

alias ll='ls -l'
alias la='ls -A'
alias l='ls -CF'
