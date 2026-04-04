import csv, os

home = os.path.expanduser("~")
csv_path = os.path.join(home, "spectrum.csv")
png_path = os.path.join(home, "spectrum.png")

freqs, powers = [], []
with open(csv_path) as f:
    next(f)
    for row in csv.reader(f):
        freqs.append(float(row[0]))
        powers.append(float(row[1]))

try:
    import matplotlib.pyplot as plt
    plt.plot(freqs, powers)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Power (dBFS)')
    plt.title('SDR Spectrum @ 94.3 MHz')
    plt.grid(True)
    plt.savefig(png_path)
    plt.show()
    print(f'Saved {png_path}')
except ImportError:
    peak = max(zip(powers, freqs), key=lambda x: x[0])
    print(f'Peak: {peak[1]/1e3:.1f} kHz offset, {peak[0]:.1f} dBFS')