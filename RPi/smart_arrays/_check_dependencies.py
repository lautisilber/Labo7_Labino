try:
    import uncertainties
    uncertainties_exists = True
except ModuleNotFoundError:
    uncertainties_exists = False
