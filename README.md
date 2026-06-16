15 minuutin run 
```
--- top 5 asks (lowest first) ---
 ask: 66416.85 qty: 5.09938
 ask: 66416.86 qty: 0.00032
 ask: 66416.87 qty: 0.00016
 ask: 66417.15 qty: 0.00016
 ask: 66418.66 qty: 0.004
--- top 5 bids (highest first) ---
 bid: 66416.84 qty: 0.49344
 bid: 66416.83 qty: 0.00088
 bid: 66416.01 qty: 0.00016
 bid: 66416.00 qty: 0.02416
 bid: 66415.94 qty: 8e-05
 --- raw spread 6641685 b: 6641684--

--- spread: 0.01 ---

========== HFT PROFILER REPORT ==========
stage           calls     avg (us)    min (us)    max (us)    total (ms)    
----------------------------------------------------------------------------
ws_read         10608     703.8       50          483437      7466.4        
json_parse      10608     95.5        16          1704        1012.7        
orderbook_upd   269562    6.1         0           445314      1636.3        
pool_alloc      269562    0.2         0           2305        44.3          
pool_dealloc    269562    0.0         0           99          1.4           
----------------------------------------------------------------------------
memory pool:
  allocated now : 1 blocks
  peak allocated: 1 blocks
  pool capacity : 8000 blocks
order book:
  bid levels    : 1404
  ask levels    : 1451
=========================================
```
