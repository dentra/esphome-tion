#!/usr/bin/env python3
"""
Source article:
  https://www.sciencedirect.com/science/article/pii/S0378778823009477
"""

from dataclasses import dataclass
# 08.00.00.00.1D.00.00.0A.02.00.00


class PIController:
    def __init__(
        self,
        Kp: float,
        Ti: float,
        db: int = 0,
        Voamin: float = float("NaN"),
        Voamax: float = float("NaN"),
    ) -> None:
        # proportional gain [L/s.ppm-CO2]
        self.kp = Kp
        # integral gain [min]
        self.ti = Ti
        # dead band [ppm]
        self.db_ = db
        # minimum outdoor airflow rate [L/s]
        self.v_oa_min = Voamin
        # maximum outdoor airflow rate [L/s]
        self.v_oa_max = Voamax

        self._ib = 0.0
        self._dt0 = 0
        self._now = 0

    @property
    def now(self) -> int:
        # must return current time millis
        return self._now

    @now.setter
    def now(self, now: int) -> None:
        self._now = now

    # time step [s]
    def _dt_s(self):
        now = self.now
        res = now - self._dt0 if self._dt0 != 0 else 0
        self._dt0 = now
        return res

    # Cset - CO2 setpoint [ppm]
    # Cval - CO2 concentration [ppm]
    def update1(self, setpoint: int, current: int) -> float:
        # 7: error from setpoint [ppm]
        e: int = setpoint - current

        # 8: error from setpoint, including dead band [ppm]
        e_db: float
        if current < setpoint - self.db_:
            e_db = e - self.db_
        elif current > setpoint + self.db_:
            e_db = e + self.db_
        else:
            e_db = 0

        # 9: integral error [min.ppm-CO2]
        ie: float = self._ib + (self._dt_s() / 60) * e_db

        def box(k, val):
            return -k * (e_db + val)

        # 10: candidate outdoor airflow rate [L/s]
        v_oa_c: float = box(self.kp, ie / self.ti)

        self.IeMax = False
        Ibv = "="
        # 11: integral error with anti-integral windup [min.ppm-CO2]
        ib: float
        if v_oa_c < self.v_oa_min:
            ib = box(self.ti, self.v_oa_min / self.kp)
            Ibv = "<"
        elif v_oa_c > self.v_oa_max:
            ib = box(self.ti, self.v_oa_max / self.kp)
            Ibv = ">"
            # self.IeMax = True
        else:
            ib = ie
            Ibv = "?"
        self._ib = ib

        # 12: outdoor airflow rate [L/s]
        v_oa: float = box(self.kp, ib / self.ti)

        print(
            # f"dt={dt}," +
            # f"now={int(now/1000/60):3}, "
            +f"e={e:4}, Edb={int(e_db):4},"
            + f" sp={setpoint}, ppm={current:3}, Voac={v_oa_c:8.02f},"
            # + f" Voamin={Voamin_:.2f}, Voamax={Voamax_:.2f},"
            # + f" Doac={Dat.cdoa(Voac):d}, cspd={Dat.cspd(Voac):d},"
            + f" I={ie:8.2f},"
            + f" Ib={ib:8.2f} {Ibv},"
            + f" Voa={v_oa:5.2f},"
            + f" Doa={Dat.cdoa(v_oa):3},"
            + f" spd={Dat.cspd(v_oa)}"
        )

        return v_oa

    def update2(self, setpoint: int, current: int) -> float:
        error = setpoint - current
        kp = self.kp
        ki = self.ti / 60 / 1000
        dt = self._dt_s() * 1000

        proportional_term = kp * error
        new_integral = error * dt * ki

        accumulated_integral = self._ib + new_integral
        self._ib = accumulated_integral

        integral_term = accumulated_integral

        derivative_term = 0

        output = proportional_term + integral_term + derivative_term

        e_db = error
        ie = integral_term
        v_oa_c = output
        v_oa = v_oa_c

        print(
            # f"dt={dt}," +
            # f"now={int(now/1000/60):3}, "
            +f"e={error:4}, Edb={int(e_db):4},"
            + f" sp={setpoint}, ppm={current:3}, Voac={v_oa_c:8.02f},"
            # + f" Voamin={Voamin_:.2f}, Voamax={Voamax_:.2f},"
            # + f" Doac={Dat.cdoa(Voac):d}, cspd={Dat.cspd(Voac):d},"
            + f" I={ie:8.2f},"
            # + f" Ib={Ib:8.2f} {Ibv},"
            # + f" Voa={Voa:5.2f},"
            # + f" Doa={Dat.cdoa(Voa):3},"
            # + f" spd={Dat.cspd(Voa)}"
        )

        return output

    def update(self, setpoint: int, current: int) -> float:
        return self.update2(setpoint, current)


pa4s = [0, 30, 45, 60, 75, 90, 120]
paLt = [0, 20, 30, 40, 50, 60, 80]
pa3s = [0, 15, 30, 50, 60, 75, 100]
paO2 = [0, 35, 60, 75, 120]
pa = pa4s


@dataclass
class Dat:
    tm: int
    ppm: int
    Voa: float
    Doa: int
    spd: int

    def __init__(self, tm, ppm, Voa) -> None:
        self.tm = tm
        self.ppm = ppm
        self.Voa = Voa

    def __setattr__(self, prop, val):
        if prop == "Voa":
            self.Doa = Dat.cdoa(val)
            self.spd = Dat.cspd(val)
        super().__setattr__(prop, val)

    @staticmethod
    def cdoa(Voa) -> float:
        a = 3.6
        b = 0.5
        # damper position for outdoor air intake
        Doa = a * Voa + b
        return int(Doa)

    @staticmethod
    def cspd(Voa) -> int:
        Doa = int(Dat.cdoa(Voa))
        for index, x in enumerate(pa[1:]):
            if Doa < x:
                return index
        return len(pa) - 1


Voa_min = pa[1]
Voa_max = pa[6]
Voa_min /= 3.6
Voa_max /= 3.6
deadband = 1
Voa_min = -10000
Voa_max = 10000

print(f"Voamin={Voa_min:.2f}, Voamax={Voa_max:.2f}, deadband={deadband}")
pi = PIController(0.076, 8, deadband, Voa_min, Voa_max)
# pi = PIController(0.05, 0.01, deadband, Voa_min, Voa_max)


setpoint = 600
ppm_step = 3
ppm_min = setpoint - 100
ppm_max = setpoint + 200
ppm_min -= ppm_min % 3  # addon to reach setpoint
t_step = 1000 * 60 * 1


def run_test():
    data: list[Dat] = []

    tm = 0

    for ppm in range(ppm_min, ppm_max + 1, ppm_step):
        tm += t_step
        pi.now = tm
        Voa = pi.update(setpoint, ppm)
        # print(f"set={setpoint}, ppm={ppm}, Voa={Voa:.4f}, Doa={Doa:.4f}")
        data.append(Dat(tm / 60000, ppm, Voa))

    for ppm in reversed(range(setpoint, ppm_max + 1, ppm_step)):
        tm += t_step
        pi.now = tm
        Voa = pi.update(setpoint, ppm)
        # print(f"set={setpoint}, ppm={ppm}, Voa={Voa:.4f}, Doa={Doa:.4f}")
        data.append(Dat(tm / 60000, ppm, Voa))

    tm += t_step * 30
    for idx, _ in enumerate(range(1, 10)):
        tm += t_step
        pi.now = tm
        Voa = pi.update(
            setpoint, setpoint + (deadband * 1 if idx % 2 == 0 else -deadband * 2)
        )
        # print(f"set={setpoint}, ppm={ppm}, Voa={Voa:.4f}, Doa={Doa:.4f}")
        data.append(Dat(tm / 60000, ppm, Voa))

    return data


if __name__ == "__main__":
    run_test()
