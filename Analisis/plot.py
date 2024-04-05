import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
import matplotlib.dates as mdates
from matplotlib.patches import Rectangle

### CONFIG

SHOW_INDICES = [0,1]
SHOW_PERIOD = 4

###

FNAME = 'data.csv'

def str_to_datetime(s: str, format: str='%Y-%m-%d_%H-%M-%S') -> datetime:
    return datetime.strptime(s, format)

def str_to_epoch(s: str, format: str='%Y-%m-%d_%H-%M-%S') -> float:
    dt = str_to_datetime(s)
    return dt.timestamp()

data_pd = pd.read_csv(FNAME, sep=',')
n_balanzas = len([c for c in data_pd.columns if 'balanza_std' in c])

# filter data
new_n_balanzas = n_balanzas
for i_drop in (i for i in range(n_balanzas) if i not in SHOW_INDICES):
    data_pd.drop(f'balanza_avg_{i_drop+1}', 'columns')
    data_pd.drop(f'balanza_std_{i_drop+1}', 'columns')
    data_pd.drop(f'n_filtered_{i_drop+1}', 'columns')
    data_pd.drop(f'grams_threshold_{i_drop+1}', 'columns')
    new_n_balanzas -= 1
n_balanzas = new_n_balanzas
del new_n_balanzas

data_pd['datetime'] = data_pd['time'].apply(str_to_datetime)
data_pd['epoch'] = data_pd['time'].apply(str_to_epoch)

def split_data_in_sessions(data: pd.DataFrame, time_diff_s: int=5*60) -> list[pd.DataFrame]:
    # time_diff_s es la cantidad de segunos de diferencia para considerarlo como dos sesiones de riego distintas
    diffs = data_pd['epoch'].diff()
    where_to_cut = [0] + list(np.where(diffs > time_diff_s)[0]) + [len(data_pd['epoch'])]
    idx_groups = list(list(i for i in range(where_to_cut[j], where_to_cut[j+1])) for j in range(len(where_to_cut)-1))
    water_sessions = list()
    for idx_group in idx_groups:
        water_sessions.append(data_pd.loc[idx_group])
    return water_sessions

split_data_pd = split_data_in_sessions(data_pd)
data = split_data_pd[SHOW_PERIOD]


### plotting
def lighten_color(color, amount=0.5):
    # > 1 is darker
    # < 1 is lighter
    # https://stackoverflow.com/questions/37765197/darken-or-lighten-a-color-in-matplotlib
    """
    Lightens the given color by multiplying (1-luminosity) by the given amount.
    Input can be matplotlib color string, hex string, or RGB tuple.

    Examples:
    >> lighten_color('g', 0.3)
    >> lighten_color('#F034A3', 0.6)
    >> lighten_color((.3,.55,.1), 0.5)
    """
    import matplotlib.colors as mc
    import colorsys
    try:
        c = mc.cnames[color]
    except:
        c = color
    c = colorsys.rgb_to_hls(*mc.to_rgb(c))
    return colorsys.hls_to_rgb(c[0], 1 - amount * (1 - c[1]), c[2])

fig, (ax1, ax2, ax3) = plt.subplots(ncols=1, nrows=3, sharex=True, figsize=(14, 7))
mat_dates = mdates.date2num(data['datetime'])
dx_dates = np.mean(np.diff(mat_dates))
lines = list()

for n in SHOW_INDICES:
    # plot pesos
    line, *_ = ax1.errorbar(mat_dates, data[f'balanza_avg_{n+1}'].to_numpy(), xerr=0, yerr=data[f'balanza_std_{n+1}'].to_numpy(), fmt='-o', label=f'balanza {n+1}', zorder=3, alpha=.8)
    lines.append(line)

    # plot riegos
    col = line.get_color()
    for d in data['datetime'].where(data[f'balanza_pump_state_{n+1}'] == 1):
        ax1.axvline(mdates.date2num(d), c=col, alpha=.3)
    
    # plot goals
    if f'balanza_goals_{n+1}' in data.columns:
        goal = data[f'balanza_goals_{n+1}']
        if f'balanza_thresholds_{n+1}' in data.columns:
            ax1.axhspan(goal - data[f'balanza_thresholds_{n+1}'], goal)
        else:
            ax1.axhline(goal)

    # ax1.plot(mat_dates, data[f'grams_goals_{n+1}'], color=col, alpha=.5, zorder=2)
    goals = data[f'grams_goals_{n+1}'].to_numpy()
    thresholds = data[f'grams_threshold_{n+1}'].to_numpy()
    dark_color = lighten_color(col, 1.3)
    if not any(np.abs(goals-thresholds)):
        ax1.plot(mat_dates, goals, label=f'goal {n+1}', alpha=.8, color=dark_color, zorder=2)
    else:
        ax1.fill_between(mat_dates, goals, goals-thresholds, label=f'goal {n+1}', alpha=.8, color=dark_color, zorder=2)

    ax2.bar(mat_dates, data[f'n_filtered_{n+1}'], width=dx_dates, color=lighten_color(col, 1.4), alpha=.3, zorder=1)

ax3.plot(mat_dates, data['hum'], '-', color='tab:blue')
ax4 = ax3.twinx()
ax4.plot(mat_dates, data['temp'], '-', color='tab:orange')

ax2.set_ylim((0, 50.1))

for ax in (ax1, ax2, ax3):
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
    ax.xaxis.set_major_locator(mdates.HourLocator(range(0, 24)))
    ax.grid(True)


legend = plt.legend(loc='upper right')

plt.show()


