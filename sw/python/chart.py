import matplotlib.pyplot as plt

numbers = []
with open('../kc1fsz-rc1/tests/clip-2b.txt', 'r') as file:
    for line in file:
        # Assuming numbers are space-separated on each line
        for num_str in line.strip().split():
            try:
                numbers.append(float(num_str)) # Use float for potential decimals
            except ValueError:
                print(f"Skipping non-numeric value: {num_str}")
        if len(numbers) > 512:
            break


# Assuming 'numbers' list contains the data read from the file
plt.figure(figsize=(10, 6)) # Optional: set figure size
plt.plot(numbers, marker='o', linestyle='-', color='blue') # Example: line plot
plt.title('Numbers from File')
plt.xlabel('Index')
#plt.yscale('log') 
plt.ylabel('Value')
plt.grid(True) # Optional: add a grid
plt.show()