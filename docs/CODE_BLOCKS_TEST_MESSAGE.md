# Code-block validation sample

Use this only as a convenient target-test payload for the dedicated code-block tranche. It is ordinary public developer documentation, not a continuity/handoff file.

````text
Hello from Pentium 4 :D

```python
import os

def main():
    print("I want MSN Messenger back!<3")
    very_long_name = "0123456789_0123456789_0123456789_0123456789_0123456789_0123456789"

if __name__ == "__main__":
    main()
```

Back to normal text <3

```c
#include <stdio.h>

int main(void) {
    printf(":) remains literal code\\n");
    return 0;
}
```

And normal mode gets the smile again :)
````

Expected presentation:

- separate role header,
- prose before the first code container,
- a `Python` code header with Copy button,
- literal `<3` inside Python code,
- prose after the first code container with graphical `<3`,
- a second `C` code container,
- literal `:)` inside C code,
- graphical `:)` in the final prose,
- horizontal overflow available for the deliberately long Python assignment line.
