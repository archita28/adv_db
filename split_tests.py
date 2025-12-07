#!/usr/bin/env python3

import re
import os

# Create tests directory
os.makedirs('tests', exist_ok=True)

# Read the test suite file
with open('test_suite.txt', 'r') as f:
    content = f.read()

# Split by test sections - look for "// Test X" pattern
test_pattern = r'// Test ([\d.]+)\n.*?(?=(?:// Test [\d.]+|// ====|$))'
matches = re.finditer(test_pattern, content, re.DOTALL)

for match in matches:
    test_num = match.group(1)
    test_content = match.group(0)
    
    # Extract just the commands (skip comment lines)
    lines = test_content.split('\n')
    commands = []
    
    for line in lines:
        line = line.strip()
        # Skip empty lines and comments
        if line and not line.startswith('//'):
            commands.append(line)
    
    # Write to file
    if commands:
        filename = f'tests/test{test_num}.txt'
        with open(filename, 'w') as f:
            f.write('\n'.join(commands) + '\n')
        print(f'Created {filename}')

print(f'\nDone! Created {len(os.listdir("tests"))} test files')