import time

def load_dataset(file_path):
    dataset = []
    with open(file_path, 'r') as f:
        for line in f:
            transaction = set(map(int, line.strip().split()))
            dataset.append(transaction)
    return dataset

def get_frequent_1_itemsets(dataset, min_sup):
    counts = {}
    for transaction in dataset:
        for item in transaction:
            counts[item] = counts.get(item, 0) + 1
    n = len(dataset)
    return {frozenset([item]): count for item, count in counts.items() 
            if (count / n) >= min_sup}

def generate_candidates(prev_frequent_itemsets, k):
    """
    OPTIMIZATION 1: ADVANCED PRUNING
    Uses the Apriori property: If any subset of a candidate is infrequent, 
    the candidate itself cannot be frequent.
    """
    candidates = set()
    list_frequent = list(prev_frequent_itemsets)
    
    # Step A: Join Step
    for i in range(len(list_frequent)):
        for j in range(i + 1, len(list_frequent)):
            l1, l2 = sorted(list(list_frequent[i])), sorted(list(list_frequent[j]))
            if l1[:k-2] == l2[:k-2]:
                candidate = list_frequent[i] | list_frequent[j]
                
                # Step B: Pruning Step (The Optimization)
                # Check if all subsets of size k-1 are frequent
                is_valid = True
                for item in candidate:
                    subset = candidate - frozenset([item])
                    if subset not in prev_frequent_itemsets:
                        is_valid = False
                        break
                if is_valid:
                    candidates.add(candidate)
    return candidates

def apriori_optimized(dataset, min_sup):
    start_time = time.time()
    n_initial = len(dataset)
    
    # Step 1: Initial pass
    current_frequent_dict = get_frequent_1_itemsets(dataset, min_sup)
    all_frequent = dict(current_frequent_dict)
    
    # Keep track of only frequent items to help with Transaction Tiding
    frequent_items_flat = set().union(*current_frequent_dict.keys())
    
    k = 2
    while current_frequent_dict:
        # Generate and prune candidates
        candidates = generate_candidates(current_frequent_dict.keys(), k)
        if not candidates:
            break
            
        candidate_counts = {can: 0 for can in candidates}
        
        """
        OPTIMIZATION 2: TRANSACTION TIDING
        We filter the dataset to only keep transactions that could 
        possibly contain a frequent k-itemset.
        """
        new_dataset = []
        for transaction in dataset:
            # Only keep items that are currently frequent
            filtered_tx = transaction.intersection(frequent_items_flat)            
            # A transaction must have at least k items to hold a k-itemset
            if len(filtered_tx) >= k:
                new_dataset.append(filtered_tx)
                for can in candidates:
                    if can.issubset(filtered_tx):
                        candidate_counts[can] += 1
        
        # Update dataset for the next level (Database Reduction)
        dataset = new_dataset
        
        # Prune based on support
        current_frequent_dict = {can: count for can, count in candidate_counts.items() 
                                 if (count / n_initial) >= min_sup}
        
        if current_frequent_dict:
            all_frequent.update(current_frequent_dict)
            # Update the list of frequent items for the next round of tiding
            frequent_items_flat = set().union(*current_frequent_dict.keys())
        
        k += 1
        
    execution_time = time.time() - start_time
    return all_frequent, execution_time