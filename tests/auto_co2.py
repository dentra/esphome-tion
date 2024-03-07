#!/usr/bin/env python3
"""
Source article:
  https://www.sciencedirect.com/science/article/pii/S0378778823009477
"""
from dataclasses import dataclass


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
        self.Kp_ = Kp
        # integral gain [min]
        self.Ti_ = Ti
        # dead band [ppm]
        self.db_ = db
        # minimum outdoor airflow rate [L/s]
        self.Voamin_ = Voamin
        # maximum outdoor airflow rate [L/s]
        self.Voamax_ = Voamax

        self.dt0_ = 0.0
        self.Ib0_ = 0.0
        self.now_ = 0
        self.IeMax = False

    @property
    def now(self) -> int:
        # must return current time millis
        return self.now_

    @now.setter
    def now(self, now: int) -> None:
        self.now_ = now

    # Cset - CO2 setpoint [ppm]
    # Cval - CO2 concentration [ppm]
    def update1(self, Cset: int, Cval: int) -> float:
        now = self.now  # current time in millis
        # time step [s]
        dt = 0 if self.dt0_ == 0 else (now - self.dt0_) / 1000
        self.dt0_ = now

        # 7: error from setpoint [ppm]
        e: int = Cset - Cval

        # 8: error from setpoint, including dead band [ppm]
        Edb: float
        if Cval < Cset - self.db_:
            Edb = e - self.db_
        elif Cval > Cset + self.db_:
            Edb = e + self.db_
        else:
            Edb = 0

        Edb = e

        # 9: integral error [min.ppm-CO2]
        Ie: float = self.Ib0_ + (dt / 60) * Edb

        def box(k, val):
            return -k * (Edb + val)

        # 10: candidate outdoor airflow rate [L/s]
        Voac: float = box(self.Kp_, Ie / self.Ti_)

        self.IeMax = False
        Ibv = "="
        # 11: integral error with anti-integral windup [min.ppm-CO2]
        Ib: float = Ie
        if Voac < self.Voamin_:
            Ib = box(self.Ti_, self.Voamin_ / self.Kp_)
            Ibv = "<"
        elif Voac > self.Voamax_:
            Ib = box(self.Ti_, self.Voamax_ / self.Kp_)
            Ibv = ">"
            # self.IeMax = True
        else:
            Ib = Ie
            Ibv = "?"
        self.Ib0_ = Ib

        # 12: outdoor airflow rate [L/s]
        Voa: float = box(self.Kp_, Ib / self.Ti_)

        print(
            # f"dt={dt}," +
            f"now={int(now/1000/60):3}, "
            + f"e={e:4}, Edb={int(Edb):4},"
            + f" sp={Cset}, ppm={Cval:3}, Voac={Voac:8.02f},"
            # + f" Voamin={Voamin_:.2f}, Voamax={Voamax_:.2f},"
            # + f" Doac={Dat.cdoa(Voac):d}, cspd={Dat.cspd(Voac):d},"
            + f" I={Ie:8.2f},"
            + f" Ib={Ib:8.2f} {Ibv},"
            + f" Voa={Voa:5.2f},"
            + f" Doa={Dat.cdoa(Voa):3},"
            + f" spd={Dat.cspd(Voa)}"
        )

        return Voa

    def update2(self, Cset: int, Cval: int) -> float:
        now = self.now  # current time in millis
        # time step [s]
        dt = 0 if self.dt0_ == 0 else (now - self.dt0_) / 1000
        self.dt0_ = now

        # 7: error from setpoint [ppm]
        e: int = Cset - Cval

        error_ = e
        kp_ = self.Kp_
        ki_ = self.Ti_ / 60 * 0.001
        dt_ = dt

        proportional_term_ = kp_ * error_
        new_integral = error_ * dt_ * ki_

        accumulated_integral_ = self.Ib0_
        accumulated_integral_ += new_integral
        self.Ib0_ = accumulated_integral_

        integral_term_ = accumulated_integral_

        derivative_term_ = 0

        output = proportional_term_ + integral_term_ + derivative_term_

        Edb = e
        Ie = integral_term_
        Voac = output
        Voa = Voac

        print(
            # f"dt={dt}," +
            f"now={int(now/1000/60):3}, "
            + f"e={e:4}, Edb={int(Edb):4},"
            + f" sp={Cset}, ppm={Cval:3}, Voac={Voac:8.02f},"
            # + f" Voamin={Voamin_:.2f}, Voamax={Voamax_:.2f},"
            # + f" Doac={Dat.cdoa(Voac):d}, cspd={Dat.cspd(Voac):d},"
            + f" I={Ie:8.2f},"
            # + f" Ib={Ib:8.2f} {Ibv},"
            # + f" Voa={Voa:5.2f},"
            # + f" Doa={Dat.cdoa(Voa):3},"
            # + f" spd={Dat.cspd(Voa)}"
        )

        return output

    def update(self, Cset: int, Cval: int) -> float:
        return self.update2(Cset, Cval)


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
