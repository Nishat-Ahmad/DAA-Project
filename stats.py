import pandas as pd
import matplotlib.pyplot as plt

# 1. Load the data
df = pd.read_csv('results.csv')

# 2. Calculate Averages (The "Rule of Three" average)
# We group by dataset, algorithm, and min_sup to get the mean of the 3 runs
avg_df = df.groupby(['dataset', 'algorithm', 'min_sup']).agg({
    'average_time': 'mean',
    'peak_ram_mb': 'mean',
    'frequent_itemsets': 'first' # This stays the same across runs
}).reset_index()

# 3. Calculate Speedup Ratio
# Formula: Speedup = (Original Apriori Time) / (Optimized Time)
pivot_time = avg_df.pivot(index=['dataset', 'min_sup'], columns='algorithm', values='average_time')
pivot_time['speedup_optimized'] = pivot_time['original_apriori'] / pivot_time['optimized_apriori']
pivot_time['speedup_sota'] = pivot_time['original_apriori'] / pivot_time['linear_table_sota']

print("--- Average Results and Speedup Ratios ---")
print(pivot_time)

# 4. Generate Scalability Curves
datasets = avg_df['dataset'].unique()

for dataset in datasets:
    subset = avg_df[avg_df['dataset'] == dataset]
    
    plt.figure(figsize=(10, 6))
    
    # Plotting Execution Time for each algorithm
    for algo in subset['algorithm'].unique():
        algo_data = subset[subset['algorithm'] == algo].sort_values('min_sup')
        plt.plot(algo_data['min_sup'], algo_data['average_time'], marker='o', label=algo)
    
    plt.title(f'Scalability Curve: Execution Time vs Min_Sup ({dataset.capitalize()})')
    plt.xlabel('Minimum Support (min_sup)')
    plt.ylabel('Average Execution Time (seconds)')
    plt.yscale('log') # Use log scale if times vary drastically
    plt.gca().invert_xaxis() # Lower support = higher complexity
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.5)
    
    # Save the figure for your IEEE report
    plt.savefig(f'{dataset}_scalability_time.png')
    plt.show()

print("\nScalability curves saved as PNG files.")