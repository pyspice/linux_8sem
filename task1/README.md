# Task1

Simple linux kernel module: kernel phonebook.

Mount: bash mount

Run: bash run

Unmount: bash unmount

Custom usage:
  add user: echo "add <name> <number>" > /dev/task1_vinokurov
  
  user info: echo "info <name>" > /dev/task1_vinokurov
  
  remove user: echo "remove <name>" > /dev/task1_vinokurov
  
  get response: cat /dev/task1_vinokurov
    
    empty message: successful add/remove operation
    
    "<name> not found": info/remove with not existing record
    
    "Error parsing query": wrong usage
