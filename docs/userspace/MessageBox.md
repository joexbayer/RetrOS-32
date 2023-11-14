# Message boxes (Userspace)

## Introduction

### Types

### Results

## Dependencies

Thread

## Usage

## Example

```c
const char* title = "Title";
const char* message = "Are you sure?";

MsgBox* msgBox = new MsgBox(title, message, MSGBOX_OK_CANCEL);
MsgBoxResult ret = msgBox->show();

delete msgBox;

if (ret == MSGBOX_CANCEL) {
    return 0;
}
```
