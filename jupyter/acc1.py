from matplotlib import pyplot as plt
import pandas as pd

def plot_accelerometer_data(df):
    df['magnitude'] = (np.sqrt(
        df[0]**2 +
        df[1]**2 +
        df[2]**2
    ) - 9.81).abs()

    plt.figure(figsize=(45.0, 4.2))
    x = range(len(df[0]))
    plt.plot(x, df['magnitude'])
    plt.plot(x, df['magnitude'].rolling(window=10).mean())
    plt.xlabel('x')
    plt.ylabel('y')
    plt.hlines(y=[0, 2, 4, 6, 8, 10], xmin=[0,], xmax=[len(df[0])], colors='purple', linestyles='--', lw=0.5)
    plt.show()

plot_accelerometer_data(pd.read_csv('2025-05-17 - t1.csv', header=None).iloc[2200:].head(10000))
plot_accelerometer_data(pd.read_csv('2025-05-31 - t2.csv', header=None).iloc[2400:].head(10000))
plot_accelerometer_data(pd.read_csv('2025-05-31 - f1.csv', header=None).iloc[1700:].head(10000))