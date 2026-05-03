import time

def load_dataset(file_path):
    """Reads the FIMI .dat files (space-separated integers)."""
    dataset = []
    with open(file_path, 'r') as f:
        for line in f:
            # Convert each line into a set of integers
            transaction = set(map(int, line.strip().split()))
            dataset.append(transaction)
    return dataset

def get_frequent_1_itemsets(dataset, min_sup):
    """Finds all single items that meet the minimum support."""
    counts = {}
    for transaction in dataset:
        for item in transaction:
            counts[item] = counts.get(item, 0) + 1
            
    n = len(dataset)
    # Filter by support threshold: (count / total_transactions) >= min_sup
    return {frozenset([item]): count for item, count in counts.items() 
            if (count / n) >= min_sup}

def generate_candidates(prev_frequent_itemsets, k):
    """
    Candidate Generation: Joins frequent itemsets of size k-1 
    to create candidates of size k.
    """
    candidates = set()
    list_frequent = list(prev_frequent_itemsets)
    
    for i in range(len(list_frequent)):
        for j in range(i + 1, len(list_frequent)):
            # Join step: if the first k-2 elements are the same, join them
            l1, l2 = list(list_frequent[i]), list(list_frequent[j])
            l1.sort(); l2.sort()
            if l1[:k-2] == l2[:k-2]:
                candidates.add(list_frequent[i] | list_frequent[j])
    return candidates

def apriori(dataset, min_sup):
    """Main Apriori loop."""
    start_time = time.time()
    
    # Step 1: Find Frequent 1-itemsets
    frequent_itemsets = get_frequent_1_itemsets(dataset, min_sup)
    all_frequent = dict(frequent_itemsets)
    current_frequent = frequent_itemsets
    
    k = 2
    while current_frequent:
        # Step 2: Generate Candidates of size k
        candidates = generate_candidates(current_frequent.keys(), k)
        
        # Step 3: Support Counting (Scan Database)
        candidate_counts = {can: 0 for can in candidates}
        for transaction in dataset:
            for can in candidates:
                if can.issubset(transaction):
                    candidate_counts[can] += 1
        
        # Step 4: Prune infrequent candidates
        n = len(dataset)
        current_frequent = {can: count for can, count in candidate_counts.items() 
                            if (count / n) >= min_sup}
        
        all_frequent.update(current_frequent)
        k += 1
        
    execution_time = time.time() - start_time
    return all_frequent, execution_time

# Usage Example:
# data = load_dataset('chess.dat')
# results, duration = apriori(data, min_sup=0.8)