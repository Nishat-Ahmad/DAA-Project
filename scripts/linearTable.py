import time
from collections import defaultdict

class LinearTableMiner:
    def __init__(self, min_sup_ratio):
        self.min_sup_ratio = min_sup_ratio
        self.frequent_itemsets = {}

    def mine(self, dataset):
        start_time = time.time()
        n = len(dataset)
        min_sup_count = self.min_sup_ratio * n

        # Step 1: Count 1-itemsets and filter frequent items
        item_counts = defaultdict(int)
        for transaction in dataset:
            for item in transaction:
                item_counts[item] += 1
        
        # Sort items by frequency (descending) - common SOTA optimization
        frequent_items = sorted(
            [item for item, count in item_counts.items() if count >= min_sup_count],
            key=lambda x: item_counts[x], reverse=True
        )
        
        # Map items to their rank for faster processing
        item_to_rank = {item: i for i, item in enumerate(frequent_items)}
        
        # Step 2: Build the Linear Table (Filtered & Sorted Database)
        linear_table = []
        for transaction in dataset:
            # Only keep frequent items and sort them by their rank
            filtered_tx = sorted(
                [item_to_rank[item] for item in transaction if item in item_to_rank]
            )
            if filtered_tx:
                linear_table.append(filtered_tx)

        # Step 3: Recursive Mining on the Table
        self._mine_recursive(linear_table, [], frequent_items, min_sup_count)
        
        execution_time = time.time() - start_time
        return self.frequent_itemsets, execution_time

    def _mine_recursive(self, current_table, prefix, items_list, min_sup):
        # Iterate through items in the current projected table
        # We process from the least frequent to most frequent (bottom-up)
        for i in range(len(items_list) - 1, -1, -1):
            new_prefix = prefix + [items_list[i]]
            
            # Count support of current item in the projected table
            support = 0
            new_table = []
            for transaction in current_table:
                if i in transaction:
                    support += 1
                    # Optimization: Only collect items "ahead" of current item for projection
                    idx = transaction.index(i)
                    if idx > 0:
                        new_table.append(transaction[:idx])
            
            if support >= min_sup:
                self.frequent_itemsets[frozenset(new_prefix)] = support
                
                # If there are items left to mine in this projection, recurse
                if new_table:
                    # The new items list is just the subset of items appearing in new_table
                    self._mine_recursive(new_table, new_prefix, items_list[:i], min_sup)

# Usage for your project:
# miner = LinearTableMiner(min_sup_ratio=0.6)
# freq_sets, duration = miner.mine(dataset)