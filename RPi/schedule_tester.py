from systems import System, IntensityConfig, Position, SerialManagerInfo, BalanzasInfo, WateringSchedule, WateringScheduleStep, CameraControllerInfo
from dummy_classes.dummy_serial import Serial
from smart_arrays import SmartArrayInt, SmartArrayBool, SmartArrayFloat
import os
from config import TEST
from typing import Optional
from dataclasses import dataclass
try:
    from tqdm import tqdm
    _have_tqdm = True
except ImportError:
    _have_tqdm = False

@dataclass(frozen=True)
class ScheduleTesterSequenceScheduleStep:
    '''This are all weights that will be produced to test the system's response during a single Schedule step (the last weight should satisfy the next schedule step)'''
    microsteps: tuple[float,...]

@dataclass()
class ScheduleTesterSequence:
    '''This are the ScheduleTesterSequenceScheduleStep that will be produced for each schedule step of a balanza'''
    steps: tuple[ScheduleTesterSequenceScheduleStep,...]
    cyclic: bool=False
    curr_step_n: int=0
    curr_microstep_n: int=0

    def get_curr_step(self) -> ScheduleTesterSequenceScheduleStep:
        return self.steps[self.curr_step_n]
    def get_curr_weight(self) -> float:
        return self.get_curr_step().microsteps[self.curr_microstep_n]

# TODO: agregar que las sequences no se sincronicen exactamente con los schedule steps sino que por cada schedule step haya un sequence step
# De esta manera hay un peso para cada step y un peso de transicion
class ScheduleTester:
    def __init__(self, system: System, sequences: Optional[tuple[ScheduleTesterSequence,...]]=None, ascii: bool=False) -> None:
        '''
        La idea de esta clase es poder testear el funcionamiento de un scedule, sin tener que esperar a que efectivamente ocurra.
        Para ello se debe proporcionar, sumado al System, una "test_sequence", que es son listas de pesos que debe reportar el
        dummy_serial, para simular pesos medidos (una lista por maceta).

        Entonces, lo que hace es cambiar el peso reportado por el dummy_serial por cada nuevo ScheduleStep.

        Tambien la idea es que las acciones que toma el System se documenten de forma inteligible para poder analizar y asegurar
        que el programa (schedule) introducido es el deseado.

        Paralbras clave:
            - schedule: son los pasos de pesos objetivo que se proponen para cada maceta.
            - sequence: es la secuencia de pesos a reportar por el dummy_serial para testear el schedule.
              La secuencia funcionara de la siguiente manera.
              Al principio se usara el primer step de la sequence para el primer step del schedule. Cuando se complete el step del
              schedule y se pase al siguiente, se usara un paso de la sequence 'intermedio' que debe estar asignado por un tick. Luego se usara
              El siguiente step de la sequence, que debera cumplir con las condiciones del siguiente step del schedule. De esta manera, el paso
              intermedio sirve para documentar el comportamiento del sistema durante los periodos de transicion entre schedule steps
              Este paso intermedio debe usarse para demostrat como se comportara el sistema 
        '''
        
        self.system = system

        # assertions
        assert isinstance(self.system.serial_manager.serial, Serial)

        self.serial = self.system.serial_manager.serial
        
        self.schedules = tuple(sch for sch in self.system.watering_schedules)
        assert all(len(s.steps) for s in self.schedules)
        self.schedule_lengths = SmartArrayInt(sch.n_steps for sch in self.schedules)

        self.n_balanzas = self.system.n_balanzas
        # el paso actual en que se encuentra cada schedule (la diferencia con el que se puede conseguir de
        # los schedules mismos es que cuando este y el de los schedules esten desincronizados, sabremos
        # que ocurrio un schedule step. esto es importante porque hay algunos steps que dependen del tiempo)
        self.schedule_steps: SmartArrayInt = SmartArrayInt(sch.current_step + int(sch.cyclic) for sch in self.schedules)
        self.finished_schedules = SmartArrayBool.zeros(self.n_balanzas)

        if sequences is None:
            seq: list[ScheduleTesterSequence] = list()
            for n in range(self.n_balanzas):
                ws = self.schedules[n]
                seq_sched_steps = [ScheduleTesterSequenceScheduleStep((
                    ws.steps[0].weight/2,
                    ws.steps[0].weight + (.5 if ws.steps[0].max_weight_difference is None else ws.steps[0].max_weight_difference/2)
                ))]
                if len(ws.steps) > 1:
                    for wss, prev_wss in zip(ws.steps[1:], ws.steps[0:-1]):
                        seq_sched_steps.append(ScheduleTesterSequenceScheduleStep((
                            (prev_wss.weight + wss.weight) / 2,
                            ((wss.weight + (.5 if wss.max_weight_difference is None else wss.max_weight_difference/2))
                             if wss.weight > prev_wss.weight else
                             (wss.weight - (.5 if wss.max_weight_difference is None else wss.max_weight_difference/2))
                            )
                        )))
                seq.append(ScheduleTesterSequence(tuple(seq_sched_steps)))
            self.sequences = tuple(seq)
        else:
            self.sequences = sequences

        self.sequence_lengths = SmartArrayInt(len(seq.steps) for seq in self.sequences)

        # assertions
        assert len(self.sequences) == self.n_balanzas

        self.fname = os.path.join(os.path.dirname(self.system.save_dir), 'test.txt')
        if os.path.isfile(self.fname):
            os.remove(self.fname)
        self.document_step: int = 0
        self.ascii = ascii

        # poner los primeros pesos en serial
        for i, s in enumerate(self.sequences):
            self.serial.hx_reading_change_single(s.get_curr_weight(), i)

        # system begin
        self.system.begin()

    def update_next_sequence_step(self) -> SmartArrayBool:
        steps_made = SmartArrayBool.zeros(self.n_balanzas)
        for i in range(self.n_balanzas):
            if self.finished_schedules[i]: continue
            if self.schedules[i].n_cycles >= 1:
                self.finished_schedules[i] = True
            if self.schedule_steps[i] != self.schedules[i].current_step: # cambio de schedule step
                self.sequences[i].curr_step_n += 1
                if self.sequences[i].curr_step_n >= len(self.sequences[i].steps):
                    if self.sequences[i].cyclic:
                        self.sequences[i].curr_step_n = 0
                    else:
                        self.sequences[i].curr_step_n = len(self.sequences[i].steps) - 1
                self.sequences[i].curr_microstep_n = 0
                steps_made[i] = True
                self.schedule_steps[i] = self.schedules[i].current_step
            else:
                self.sequences[i].curr_microstep_n += 1
                if self.sequences[i].curr_microstep_n >= len(self.sequences[i].get_curr_step().microsteps):
                    self.sequences[i].curr_microstep_n = len(self.sequences[i].get_curr_step().microsteps) - 1
                
            # obtener el objetivo de peso y asignarlo
            next_goal = self.sequences[i].get_curr_weight()
            self.serial.hx_reading_change_single(next_goal, i)

        return steps_made
    
    def document_tick(self) -> None:
        u = '/' if self.ascii else chr(8599)
        d = '\\' if self.ascii else chr(8600)
        m = '-' if self.ascii else chr(8594)
        weights = self.system.last_weights # los ultimos pesos
        if weights is None: return
        if not all(w for w in self.system.history_watering): return
        watered = tuple('Si' if w[-1] else 'No' for w in self.system.history_watering) # que macetas se regaron
        goals = SmartArrayFloat(sch.current_goal for sch in self.schedules) # cuales son los pesos objetivos actuales
        on_goals = tuple('Si' if sch._in_goal(w) else 'No' for sch, w in zip(self.schedules, weights)) # si esta en el peso objetivo
        derivadas = tuple(m if o else u if g > w else d for w, g, o in zip(weights, goals, (o == 'Si' for o in on_goals))) # 0 si on_goal, 1 si hay que aumentar peso para llegar al objetivo, -1 si hay que perder peso para llegar al objetivo
        step_sched = tuple(str(i) if not f else 'End' for i, f in zip(self.schedule_steps, self.finished_schedules)) # numero de step de los schedules
        step_sequence = tuple(s.curr_step_n for s in self.sequences) # numero de step de la sequence (pesos para testear)
        microstepstep_sequence = tuple(s.curr_microstep_n for s in self.sequences) # numero de microstepstep de la sequence (pesos para testear)

        first_time = not os.path.isfile(self.fname)
        if self.ascii:
            h_div_u = '+--------+-----------+----------+--------+----------+---------+-----------+------------+-----------+----------------+\n'
            h_div_m = '+--------+-----------+----------+--------+----------+---------+-----------+------------+-----------+----------------+\n'
            h_div_d = '+--------+-----------+----------+--------+----------+---------+-----------+------------+-----------+----------------+\n'
            vert = '|'
        else:
            _hor = chr(9472)
            _top_left = chr(9484)
            _top_right = chr(9488)
            _top = chr(9516)
            _bottom = chr(9524)
            _bottom_right = chr(9496)
            _bottom_left = chr(9492)
            _center = chr(9532)
            _left = chr(9500)
            _right = chr(9508)
            vert = chr(9474)
            h_div_u = _top_left + _hor*8 + _top + _hor*11 + _top + _hor*10 + _top + _hor*8 + _top + _hor*10 + _top + _hor*9 + _top + _hor*11 + _top + _hor*12 + _top + _hor*11 + _top + _hor*16 + _top_right + '\n'
            h_div_m = _left + _hor*8 + _center + _hor*11 + _center + _hor*10 + _center + _hor*8 + _center + _hor*10 + _center + _hor*9 + _center + _hor*11 + _center + _hor*12 + _center + _hor*11 + _center + _hor*16 + _right + '\n'
            h_div_d = _bottom_left + _hor*8 + _bottom + _hor*11 + _bottom + _hor*10 + _bottom + _hor*8 + _bottom + _hor*10 + _bottom + _hor*9 + _bottom + _hor*11 + _bottom + _hor*12 + _bottom + _hor*11 + _bottom + _hor*16 + _bottom_right + '\n'
        encoding = 'ascii' if self.ascii else 'utf-8'
        with open(self.fname, 'a', encoding=encoding) as f:
            if first_time:
                f.write(h_div_u)
                w = f'{vert} n_tick {vert} n_balanza {vert}   peso   {vert} regado {vert} objetivo {vert} on_goal {vert} tendencia {vert} paso_sched {vert} paso_test {vert} micropaso_step {vert}\n'
                f.write(w)
                f.write(h_div_d)

            f.write(h_div_u)
            for i in range(self.n_balanzas):
                w = f'{vert}{self.document_step:^8d}{vert}{i+1:^11d}{vert}{weights[i]:^10.2f}{vert}{watered[i]:^8s}{vert}{goals[i]:^10.2f}{vert}{on_goals[i]:^9s}{vert}{derivadas[i]:^11s}{vert}{step_sched[i]:^12s}{vert}{step_sequence[i]:^11d}{vert}{microstepstep_sequence[i]:^16d}{vert}\n'
                f.write(w)
                if i < self.n_balanzas - 1:
                    f.write(h_div_m)
                else:
                    f.write(h_div_d)

        self.document_step += 1

    def test_schedule(self) -> None:
        if _have_tqdm:
            it = [tqdm(total=l, position=i, desc=f'Balanza {i+1}') for i, l in enumerate(self.schedule_lengths)] # type: ignore
        else:
            it = None

        # while True:

        while not all(self.finished_schedules):
            self.system.tick()
            steps_made = self.update_next_sequence_step()
            self.document_tick()
            if it:
                for s, f, i in zip(steps_made, self.finished_schedules, it):
                    if i.n >= i.total: continue
                    if s or f:
                        i.update()

        if it:
            for i in it:
                i.close()


def main() -> None:
    if not TEST:
        raise Exception('TEST variable has to be set to True to make a schedule test!')

    sys = System(
        positions=(
            Position(0, 179, IntensityConfig(1600, 3000, 15, 5), IntensityConfig(53, 75, 30, 15)),    # pos 1
            Position(0, 1, IntensityConfig(1600, 3400, 15, 5), IntensityConfig(55, 80, 30, 15)),      # pos 2
            Position(4800, 1, IntensityConfig(1700, 3400, 15, 5), IntensityConfig(53, 80, 30, 15)),   # pos 3
            Position(5000, 172, IntensityConfig(1700, 3400, 15, 5), IntensityConfig(53, 80, 30, 15)),  # pos 4
            Position(9700, 162, IntensityConfig(1900, 3500, 15, 2), IntensityConfig(53, 80, 30, 15)),  # pos 5
            Position(9700, 20, IntensityConfig(1800, 3400, 15, 2), IntensityConfig(53, 80, 30, 15))   # pos 6
        ),
        sm_info=SerialManagerInfo(
            port='/dev/ttyACM0',
            baud_rate=9600,
            parity='E'
        ),
        balanzas_info=BalanzasInfo(
            n_statistics=50,
            n_arduino=10
        ),
        watering_schedules=tuple(
            [WateringSchedule((WateringScheduleStep(670), WateringScheduleStep(610), WateringScheduleStep(670)), cyclic=False)]*6
            ),
        cc_info=CameraControllerInfo(),
        n_balanzas=6,
        name='system_test',
        save_dir=os.path.join('systems_test', 'system_test'),
        use_tqdm=False
    )

    st = ScheduleTester(sys, ascii=False)
    st.test_schedule()

if __name__ == '__main__':
    main()