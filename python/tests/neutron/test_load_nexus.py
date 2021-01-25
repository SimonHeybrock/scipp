from nexus_helpers import in_memory_nexus_file_with_event_data
import scipp as sc


def test_load_nexus_loads_event_data_from_nxevent_data_group():
    with in_memory_nexus_file_with_event_data() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)
    print(loaded_data)
