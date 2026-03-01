#!/bin/bash

echo "Running clang-format on all files..."
/bin/clang-format-14 -i $(find . -regex '.*\.\(cpp\|h\)$')
echo "Done."