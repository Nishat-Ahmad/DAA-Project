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

def apriori_original(dataset, min_sup):
    start_time = time.time()
    n_transactions = len(dataset)
    frequent_itemsets = {}
    candidate_count = 0

    current_frequent = get_frequent_1_itemsets(dataset, min_sup)
    frequent_itemsets.update(current_frequent)

    k = 2
    prev_frequent_keys = list(current_frequent.keys())
    while prev_frequent_keys:
        candidates = set()
        for i in range(len(prev_frequent_keys)):
            for j in range(i + 1, len(prev_frequent_keys)):
                left = sorted(prev_frequent_keys[i])
                right = sorted(prev_frequent_keys[j])
                if left[: k - 2] == right[: k - 2]:
                    candidates.add(prev_frequent_keys[i] | prev_frequent_keys[j])

        if not candidates:
            break

        candidate_count += len(candidates)
        candidate_support = {candidate: 0 for candidate in candidates}
        for transaction in dataset:
            for candidate in candidates:
                if candidate.issubset(transaction):
                    candidate_support[candidate] += 1

        current_frequent = {
            candidate: support
            for candidate, support in candidate_support.items()
            if (support / n_transactions) >= min_sup
        }

        if not current_frequent:
            break

        frequent_itemsets.update(current_frequent)
        prev_frequent_keys = list(current_frequent.keys())
        k += 1

    execution_time = time.time() - start_time
    return frequent_itemsets, execution_time, candidate_count

def apriori_optimized_with_stats(dataset, min_sup):
    start_time = time.time()
    n_initial = len(dataset)

    current_frequent_dict = get_frequent_1_itemsets(dataset, min_sup)
    all_frequent = dict(current_frequent_dict)

    frequent_items_flat = set().union(*current_frequent_dict.keys()) if current_frequent_dict else set()
    candidate_count = 0

    k = 2
    while current_frequent_dict:
        candidates = generate_candidates(current_frequent_dict.keys(), k)
        if not candidates:
            break

        candidate_count += len(candidates)
        candidate_counts = {can: 0 for can in candidates}

        new_dataset = []
        for transaction in dataset:
            filtered_tx = transaction.intersection(frequent_items_flat)
            if len(filtered_tx) >= k:
                new_dataset.append(filtered_tx)
                for can in candidates:
                    if can.issubset(filtered_tx):
                        candidate_counts[can] += 1

        dataset = new_dataset
        current_frequent_dict = {
            can: count for can, count in candidate_counts.items()
            if (count / n_initial) >= min_sup
        }

        if current_frequent_dict:
            all_frequent.update(current_frequent_dict)
            frequent_items_flat = set().union(*current_frequent_dict.keys())

        k += 1

    execution_time = time.time() - start_time
    return all_frequent, execution_time, candidate_count

def apriori_optimized(dataset, min_sup):
    all_frequent, execution_time, _ = apriori_optimized_with_stats(dataset, min_sup)
    return all_frequent, execution_time