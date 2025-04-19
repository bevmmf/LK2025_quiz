import numpy as np
import matplotlib.pyplot as plt

# 假陽性機率公式
def false_positive_rate(m, k, n):
    return (1 - np.exp(-k * n / m)) ** k

# 圖 1：P 對 m
n_fixed = 1000
k_fixed = 5
m_values = np.arange(1000, 10001, 100)
p_m = false_positive_rate(m_values, k_fixed, n_fixed)

plt.figure()
plt.plot(m_values, p_m, label=f'k={k_fixed}, n={n_fixed}')
plt.xlabel('m (bit array size)')
plt.ylabel('False Positive Rate (P)')
plt.title('P vs m')
plt.grid(True)
plt.legend()
plt.show()

# 圖 2：P 對 k
m_fixed = 10000
n_fixed = 1000
k_values = np.arange(1, 21, 1)
p_k = false_positive_rate(m_fixed, k_values, n_fixed)

plt.figure()
plt.plot(k_values, p_k, label=f'm={m_fixed}, n={n_fixed}')
plt.xlabel('k (number of hash functions)')
plt.ylabel('False Positive Rate (P)')
plt.title('P vs k')
plt.grid(True)
plt.legend()
plt.show()

# 圖 3：P 對 n
m_fixed = 10000
k_fixed = 5
n_values = np.arange(100, 2001, 100)
p_n = false_positive_rate(m_fixed, k_fixed, n_values)

plt.figure()
plt.plot(n_values, p_n, label=f'm={m_fixed}, k={k_fixed}')
plt.xlabel('n (number of elements)')
plt.ylabel('False Positive Rate (P)')
plt.title('P vs n')
plt.grid(True)
plt.legend()
plt.show()
